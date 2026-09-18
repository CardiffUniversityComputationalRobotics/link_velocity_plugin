#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include <gz/common/Console.hh>
#include <gz/math/Vector3.hh>
#include <gz/msgs/twist.pb.h>
#include <gz/plugin/Register.hh>
#include <gz/sim/Entity.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Link.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Types.hh>
#include <gz/transport/Node.hh>
#include <sdf/Element.hh>

namespace link_velocity_plugin
{
class LinkVelocityPlugin:
  public gz::sim::System,
  public gz::sim::ISystemConfigure,
  public gz::sim::ISystemPreUpdate
{
  public: LinkVelocityPlugin() = default;

  public: void Configure(
    const gz::sim::Entity &_entity,
    const std::shared_ptr<const sdf::Element> &_sdf,
    gz::sim::EntityComponentManager &_ecm,
    gz::sim::EventManager &/*_eventMgr*/) override
  {
    this->modelEntity = _entity;

    gz::sim::Model model(_entity);
    if (!model.Valid(_ecm))
    {
      gzerr << "LinkVelocityPlugin must be attached to a model entity.\n";
      return;
    }

    if (_sdf->HasElement("link_name"))
    {
      this->linkName = _sdf->Get<std::string>("link_name");
    }

    if (_sdf->HasElement("topic"))
    {
      this->topic = _sdf->Get<std::string>("topic");
    }

    if (this->topic.empty())
    {
      this->topic = "/cmd_vel";
    }

    if (_sdf->HasElement("command_timeout"))
    {
      const auto timeoutSeconds = _sdf->Get<double>("command_timeout");
      if (timeoutSeconds >= 0.0)
      {
        this->commandTimeout =
          std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(timeoutSeconds));
      }
      else
      {
        gzwarn << "Ignoring negative LinkVelocityPlugin command_timeout ["
               << timeoutSeconds << "]. Using default 0.5s.\n";
      }
    }

    this->ResolveLink(_ecm);

    const bool subscribed = this->node.Subscribe(
      this->topic,
      &LinkVelocityPlugin::OnCmdVel,
      this);

    if (!subscribed)
    {
      gzerr << "LinkVelocityPlugin failed to subscribe to ["
            << this->topic << "].\n";
      return;
    }

    gzmsg << "LinkVelocityPlugin controlling link [" << this->linkName
          << "] from topic [" << this->topic << "] with timeout ["
          << std::chrono::duration<double>(this->commandTimeout).count()
          << "s].\n";
  }

  public: void PreUpdate(
    const gz::sim::UpdateInfo &_info,
    gz::sim::EntityComponentManager &_ecm) override
  {
    if (_info.paused)
    {
      return;
    }

    if (this->linkEntity == gz::sim::kNullEntity)
    {
      this->ResolveLink(_ecm);
      if (this->linkEntity == gz::sim::kNullEntity)
      {
        return;
      }
    }

    gz::math::Vector3d linear{0, 0, 0};
    gz::math::Vector3d angular{0, 0, 0};
    uint64_t commandSequence = 0;
    bool hasCommand = false;

    {
      std::lock_guard<std::mutex> lock(this->mutex);
      linear = this->linearCommand;
      angular = this->angularCommand;
      commandSequence = this->commandSequence;
      hasCommand = this->hasCommand;
    }

    if (commandSequence != this->lastSeenCommandSequence)
    {
      this->lastSeenCommandSequence = commandSequence;
      this->lastCommandSimTime = _info.simTime;
    }

    const bool commandExpired =
      !hasCommand ||
      (_info.simTime - this->lastCommandSimTime) > this->commandTimeout;

    if (commandExpired)
    {
      linear.Set(0, 0, 0);
      angular.Set(0, 0, 0);
    }

    gz::sim::Link link(this->linkEntity);
    link.SetLinearVelocity(_ecm, linear);
    link.SetAngularVelocity(_ecm, angular);
  }

  private: void OnCmdVel(const gz::msgs::Twist &_msg)
  {
    std::lock_guard<std::mutex> lock(this->mutex);

    this->linearCommand.Set(
      _msg.linear().x(),
      _msg.linear().y(),
      _msg.linear().z());
    this->angularCommand.Set(
      _msg.angular().x(),
      _msg.angular().y(),
      _msg.angular().z());
    this->hasCommand = true;
    ++this->commandSequence;
  }

  private: void ResolveLink(gz::sim::EntityComponentManager &_ecm)
  {
    gz::sim::Model model(this->modelEntity);
    if (!model.Valid(_ecm))
    {
      return;
    }

    this->linkEntity = model.LinkByName(_ecm, this->linkName);
    if (this->linkEntity == gz::sim::kNullEntity)
    {
      if (!this->warnedMissingLink)
      {
        gzerr << "LinkVelocityPlugin failed to find link ["
              << this->linkName << "] in model ["
              << model.Name(_ecm) << "].\n";
        this->warnedMissingLink = true;
      }
      return;
    }

    if (this->warnedMissingLink)
    {
      gzmsg << "LinkVelocityPlugin resolved link ["
            << this->linkName << "].\n";
    }
    this->warnedMissingLink = false;
  }

  private: gz::sim::Entity modelEntity{gz::sim::kNullEntity};
  private: gz::sim::Entity linkEntity{gz::sim::kNullEntity};
  private: std::string linkName{"base_link"};
  private: std::string topic{"/cmd_vel"};
  private: std::chrono::steady_clock::duration commandTimeout{
    std::chrono::duration_cast<std::chrono::steady_clock::duration>(
      std::chrono::duration<double>(0.5))};
  private: std::chrono::steady_clock::duration lastCommandSimTime{0};
  private: uint64_t commandSequence{0};
  private: uint64_t lastSeenCommandSequence{0};
  private: bool hasCommand{false};
  private: bool warnedMissingLink{false};
  private: gz::math::Vector3d linearCommand{0, 0, 0};
  private: gz::math::Vector3d angularCommand{0, 0, 0};
  private: std::mutex mutex;
  private: gz::transport::Node node;
};
}  // namespace link_velocity_plugin

GZ_ADD_PLUGIN(
  link_velocity_plugin::LinkVelocityPlugin,
  gz::sim::System,
  link_velocity_plugin::LinkVelocityPlugin::ISystemConfigure,
  link_velocity_plugin::LinkVelocityPlugin::ISystemPreUpdate)

GZ_ADD_PLUGIN_ALIAS(
  link_velocity_plugin::LinkVelocityPlugin,
  "link_velocity_plugin::LinkVelocityPlugin")
