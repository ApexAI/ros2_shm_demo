# ros2_shm_demo
This example demonstrates how to use zero-copy Shared Memory data transfer in ROS 2. The middleware used is Eclipse CycloneDDS which integrates Eclipse iceoryx for Shared Memory transfer.

Since it uses char arrays for its payload data, the example can easily be generalized to arbitrary data such as images and point clouds.

Currently the ROS message types are limited to fixed-size data types and only a subset of [QoS settings](https://github.com/ros2/rmw_cyclonedds/blob/master/shared_memory_support.md#QoS-settings) is supported.

## Installation

Install ROS 2 Galactic from source as described in https://docs.ros.org/en/galactic/Installation.html.

Clone this repository into the src folder of your ROS 2 workspace and run:

```sh
~ros2_galactic_ws$ colcon build
```

## Run the example

In the ROS 2 workspace in all terminals:

```sh
~/ros2_galactic_ws$ source install/setup.bash
```

In different terminals.

Run the RouDi middleware daemon

```sh
~/ros2_galactic_ws$ iox-roudi
```

Start the talker

```sh
~/ros2_galactic_ws$ export CYCLONEDDS_URI=file://$PWD/src/ros2_shm_demo/cyclonedds.xml
~/ros2_galactic_ws$ ros2 run ros2_shm_demo talker
```

Start the listener

```sh
~/ros2_galactic_ws$ export CYCLONEDDS_URI=file://$PWD/src/ros2_shm_demo/cyclonedds.xml
~/ros2_galactic_ws$ ros2 run ros2_shm_demo listener
```

## Valdiate Shared Memory usage

### Using the introspection

Build the [iceoryx introspection](https://github.com/ros2/rmw_cyclonedds/blob/master/shared_memory_support.md#Using-the-iceoryx-introspection).

Note that this is only supported unofficially and hence we build it manually without colcon.

Run the RouDi as in the steps before but do not start the talker and listener yet.

In another terminal navigate to the folder where the introspection client was build and run

```sh
cd src/eclipse-iceoryx/iceoryx/tools/introspection/build
~/ros2_galactic_ws/src/eclipse-iceoryx/iceoryx/tools/introspection/build$ ./iox-introspection-client --all
```

Now start the talker and listener as in [the example section](#Run-the-example).

When talker or listener are started the introspection detects new processes and publisher or subscriber ports.
While the talker is running you should see an increase in mempool usage of a specific size up to *PubHistoryCapacity* as defined in the configuration file. This happens because the publisher stores these samples in Shared Memory for late-joining subscribers.

If the listener is running as well this could increase to *SubQueueCapacity*, but only if the listener would not take the data almost immediately. Since this is what happens with event-driven subscriptions in ROS 2 we will not observe this.

### Eavesdropping with an iceoryx subscriber

Now we can also start a native iceoryx subscriber to verify data is transmitted via Shared Memory.
Such a subscriber is build by this project for verification purposes. Note that it currently has to rely on some implementation details of CycloneDDS and hence may not work if these change.

The idea is that it is possible to have an iceoryx subscriber to the specific topic name (which we can extract from the introspection) and also receive dthe data send by the talker via this subscriber.

To interpret the data, it is important to know what exactly is send and how it is serialized. In this case no serialization happens but CycloneDDS adds some header information before the actual data which we need to account for. The talker sends a samples of type ```ros2_shm_demo::msg::ShmTopic``` (the ROS type) with some 56 byte header prefix. We thus can take an offset of 56 bytes from the data we receive and interpret the sample as the ROS type (cf. *iox_subscriber.cpp*).

Note that the size of the header may change and it is not part of the CycloneDDS interface hence the iox_subscriber is tied to the current version of CycloneDDS. Since iceoryx now provides a specific interface for user headers like this, this compsensation for the header will not be necessary in the future. This would also allow to use a typed iceoryx subscriber.

We can run the example together with the introsepction as before, but now we will also start the iox_subscriber in another terminal.

```sh
~/ros2_galactic_ws$ ros2 run ros2_shm_demo iox_subscriber
```

The iceoryx subscriber receives the same data as the listener, but only if the talker sends it using Shared Memory.
If we start the talker with a Shared Memory configuration, we can observe that the data is actually send with iceoryx Shared Memory.

Conversely, not activating the configuration with

```sh
~/ros2_galactic_ws$ export CYCLONEDDS_URI=file://$PWD/src/ros2_shm_demo/cyclonedds.xml
```

before starting the talker will lead to sending the data over the network and the iceoryx subscriber will not receive any data. If this is the case, the introspection will not show any iceoryx publisher or subscriber (except for the built-in ones, which are used for the introspection itself).
