//
//
// Copyright 2016 gRPC authors.
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
//
//

#include "src/cpp/common/channel_filter.h"

#include <type_traits>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

#include <grpc/support/log.h>

#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/channel/channel_stack.h"
#include "src/core/lib/config/core_configuration.h"
#include "src/core/lib/slice/slice.h"

namespace grpc {

// MetadataBatch

void MetadataBatch::AddMetadata(const string& key, const string& value) {
  batch_->Append(key, grpc_core::Slice::FromCopiedString(value),
                 [&](absl::string_view error, const grpc_core::Slice&) {
                   gpr_log(GPR_INFO, "%s",
                           absl::StrCat("MetadataBatch::AddMetadata error:",
                                        error, " key=", key, " value=", value)
                               .c_str());
                 });
}

// ChannelData

void ChannelData::StartTransportOp(grpc_channel_element* elem,
                                   TransportOp* op) {
  grpc_channel_next_op(elem, op->op());
}

void ChannelData::GetInfo(grpc_channel_element* elem,
                          const grpc_channel_info* channel_info) {
  grpc_channel_next_get_info(elem, channel_info);
}

// CallData

void CallData::StartTransportStreamOpBatch(grpc_call_element* elem,
                                           TransportStreamOpBatch* op) {
  grpc_call_next_op(elem, op->op());
}

void CallData::SetPollsetOrPollsetSet(grpc_call_element* elem,
                                      grpc_polling_entity* pollent) {
  grpc_call_stack_ignore_set_pollset_or_pollset_set(elem, pollent);
}

namespace internal {

void RegisterChannelFilter(
    grpc_channel_stack_type stack_type, int,
    std::function<bool(const grpc_core::ChannelArgs&)> include_filter,
    const grpc_channel_filter* filter) {
  grpc_core::CoreConfiguration::RegisterBuilder(
      [stack_type, filter, include_filter = std::move(include_filter)](
          grpc_core::CoreConfiguration::Builder* builder) {
        auto& f = builder->channel_init()->RegisterFilter(stack_type, filter);
        if (include_filter) f.If(include_filter);
      });
}

}  // namespace internal

}  // namespace grpc
