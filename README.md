# link_velocity_plugin

A Gazebo Sim **system plugin** that drives a single link of a model directly from
`gz.msgs.Twist` velocity commands, with a watchdog that stops the link when commands
go stale.

Packaged as an `ament_cmake` ROS 2 package so it builds with `colcon` and registers
itself on `GZ_SIM_SYSTEM_PLUGIN_PATH` when you source the workspace.

## What it's for

You have something in the simulation that needs to *move at a commanded velocity*
without you modelling a drivetrain for it: a simulated pedestrian, a moving obstacle,
a free-floating body, a conveyor item, a camera rig on a virtual gantry.

Attach this plugin to the model, point it at a link and a topic, and publish a `Twist`.
The link moves. Stop publishing and it stops moving.

This is **kinematic** control: the plugin sets the link's velocity directly rather than
applying forces, so the commanded velocity is tracked exactly (see
[Behaviour notes](#behaviour-notes) for what that costs you).

### Compared to Gazebo's built-in `VelocityControl`

The stock `gz::sim::systems::VelocityControl` covers the common case. Reach for this
plugin when you want:

- an **arbitrary topic name** (not the fixed `/model/<name>/link/<link>/cmd_vel`),
- a **command timeout / watchdog** so a dead publisher means "stop", not "keep going forever",
- a link that may not exist yet at load time (it is resolved lazily each update).

If you don't need those, use the built-in one.

## Requirements

| | |
|---|---|
| Gazebo | Sim 8 (Harmonic) — `gz-sim8`, `gz-transport13`, `gz-msgs10`, `gz-math7`, `gz-common5`, `gz-plugin2` |
| ROS 2 | Humble (only for `ament_cmake` / `colcon`; the plugin itself has no ROS dependency) |
| Compiler | C++17 |

## Build

```bash
cd ~/ros2_ws          # your workspace root
colcon build --packages-select link_velocity_plugin
source install/setup.bash
```

Sourcing the workspace is what makes the plugin findable — the package ships an
environment hook that prepends `lib/link_velocity_plugin` to `GZ_SIM_SYSTEM_PLUGIN_PATH`.
If Gazebo reports "Failed to load system plugin", you almost certainly skipped the
`source`.

## Usage

Add the plugin to a `<model>` in your SDF/URDF world:

```xml
<model name="walking_person">
  <link name="body">
    <!-- ... -->
  </link>

  <plugin filename="LinkVelocityPlugin"
          name="link_velocity_plugin::LinkVelocityPlugin">
    <link_name>body</link_name>
    <topic>/person1/cmd_vel</topic>
    <command_timeout>0.5</command_timeout>
  </plugin>
</model>
```

Then command it over Gazebo Transport:

```bash
gz topic -t /person1/cmd_vel -m gz.msgs.Twist \
  -p 'linear: {x: 1.0}, angular: {z: 0.3}'
```

### Parameters

All SDF elements are optional.

| Element | Type | Default | Description |
|---|---|---|---|
| `link_name` | string | `base_link` | Name of the link within this model to drive. |
| `topic` | string | `/cmd_vel` | **Gazebo Transport** topic carrying `gz.msgs.Twist`. Not a ROS topic — see below. |
| `command_timeout` | double (seconds) | `0.5` | Zero the link's velocity if no new command arrives within this window of simulation time. A negative value is ignored with a warning and the default is used. Set to a large number to disable the watchdog. |

### Commanding from ROS 2

The plugin speaks Gazebo Transport, not ROS. Bridge it with `ros_gz_bridge`:

```bash
ros2 run ros_gz_bridge parameter_bridge \
  /person1/cmd_vel@geometry_msgs/msg/Twist@gz.msgs.Twist
```

```bash
ros2 topic pub /person1/cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 1.0}, angular: {z: 0.3}}"
```

## Behaviour notes

- **Frame.** Linear and angular velocities are applied in the **link's own frame**, so
  `linear.x = 1.0` means "one metre per second forward along the link's x axis",
  wherever the link is currently pointing.
- **Wrenches are ignored.** Gazebo skips forces/torques on a link whose velocity is
  commanded, for that time step. Don't expect this link to react to contacts, gravity
  or applied wrenches while the plugin is driving it. That's the trade for exact
  velocity tracking.
- **Watchdog uses simulation time.** The timeout is measured against sim time, not
  wall clock, so it behaves the same under real-time factors other than 1.0.
- **It always writes.** On every unpaused update the plugin sets a velocity — the
  commanded one, or zero if the command expired. It is not a "set it and forget it"
  impulse; it holds the link at the commanded velocity until told otherwise.
- **Paused simulation is a no-op.** No velocity is written while paused.
- **Lazy link resolution.** If `link_name` isn't found at load time, the plugin retries
  on each update and logs the error once, so models whose links appear later still work.
- **Attach it to a model.** Configuring the plugin on a non-model entity logs an error
  and does nothing.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| `Failed to load system plugin` | Workspace not sourced; `GZ_SIM_SYSTEM_PLUGIN_PATH` doesn't include `install/link_velocity_plugin/lib/link_velocity_plugin`. |
| `failed to find link [...] in model [...]` | `link_name` doesn't match a link in the model the plugin is attached to. |
| Link twitches and stops repeatedly | Publish rate is slower than `command_timeout`. Publish faster or raise the timeout. |
| Nothing moves, no errors | Publishing to the wrong topic. Check with `gz topic -l` and `gz topic -e -t <topic>`. |
| Link falls through the floor / ignores collisions | Expected — velocity-commanded links ignore wrenches. Use a physical controller if you need contact response. |

