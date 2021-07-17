// Copyright 2021 Apex.AI, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ros2_shm_demo/msg/shm_topic.hpp"

#include "iceoryx_posh/popo/untyped_subscriber.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/posix_wrapper/signal_handler.hpp"

#include <iostream>

// Internally CycloneDDS prefixes the data with a header and we have to
// account for this.
// We do not want dependencies to DDS here, hence it is hard-coded
// This is only for demo purposes and should not be done in production code!
// In the future CycloneDDS will use the new user header provided by iceoryx
// instead.
constexpr uint32_t DDS_IOX_HEADER_SIZE = 56;
using ROSTopic = ros2_shm_demo::msg::ShmTopic;

bool shutdown = false;
constexpr char APP_NAME[] = "iox-ros2-subscriber";

static void sigHandler(int) {
  // caught SIGINT or SIGTERM, now exit gracefully
  shutdown = true;
}

int main() {
  // register sigHandler
  auto signalIntGuard =
      iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
  auto signalTermGuard =
      iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);

  // initialize runtime
  iox::runtime::PoshRuntime::initRuntime(APP_NAME);

  // initialize subscriber
  // we know how the services are mapped to dds
  // note that we can use the introspection to get this info
  // Note: If not for the internal header C<cloneDDS uses, we could use a
  // subscriber with type ROSTopic here.
  iox::popo::UntypedSubscriber subscriber(
      {"DDS_CYCLONE", "ros2_shm_demo::msg::dds_::ShmTopic_", "rt/chatter"});

  auto state = subscriber.getSubscriptionState();

  if (state == iox::SubscribeState::SUBSCRIBED) {
    std::cout << "iox subscriber: Subscribed to "
                 "ros2_shm_demo::msg::dds_::ShmTopic_ rt/chatter"
              << std::endl;
  }

  while (!shutdown) {
    subscriber.take()
        .and_then([&](const void *payload) {
          // this is very brittle and only to show that iceoryx can eavesdrop on
          // CycloneDDS if Shared Memory is used
          auto *payloadWithoutDDSHeader =
              reinterpret_cast<const uint8_t *>(payload) + DDS_IOX_HEADER_SIZE;

          // we account for the header CycloneDDS currently prepends before the
          // actual ROS sample
          const ROSTopic *sample =
              reinterpret_cast<const ROSTopic *>(payloadWithoutDDSHeader);
          auto data = reinterpret_cast<const char *>(sample->data.data());

          std::cout << "iox subscriber: Received " << data << " "
                    << sample->counter << std::endl;

          subscriber.release(payload);
        })
        .or_else([](auto &result) {
          // only has to be called if the alternative is of
          // interest, i.e. if nothing has to happen when no data
          // is received and a possible error alternative is not
          // checked or_else is not needed
          if (result != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE) {
            std::cout << "Error receiving chunk." << std::endl;
          }
        });

    // use polling for simplicity
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return (EXIT_SUCCESS);
}