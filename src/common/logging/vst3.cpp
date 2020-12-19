// yabridge: a Wine VST bridge
// Copyright (C) 2020  Robbert van der Helm
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "vst3.h"

#include <public.sdk/source/vst/utility/stringconvert.h>

#include "src/common/serialization/vst3.h"

Vst3Logger::Vst3Logger(Logger& generic_logger) : logger(generic_logger) {}

void Vst3Logger::log_unknown_interface(
    const std::string& where,
    const std::optional<Steinberg::FUID>& uid) {
    if (BOOST_UNLIKELY(logger.verbosity >= Logger::Verbosity::most_events)) {
        std::string uid_string = uid ? format_uid(*uid) : "<unknown_pointer>";

        std::ostringstream message;
        message << "[unknown interface] " << where << ": " << uid_string;

        log(message.str());
    }
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const Vst3PluginProxy::Construct& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << "IPluginFactory::createComponent(cid = "
                << format_uid(Steinberg::FUID::fromTUID(request.cid.data()))
                << ", _iid = ";
        switch (request.requested_interface) {
            case Vst3PluginProxy::Construct::Interface::IComponent:
                message << "IComponent::iid";
                break;
            case Vst3PluginProxy::Construct::Interface::IEditController:
                message << "IEditController::iid";
                break;
        }
        message << ", &obj)";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const Vst3PluginProxy::Destruct& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        // We don't know what class this instance was originally instantiated
        // as, but it also doesn't really matter
        message << request.instance_id << ": FUnknown::~FUnknown()";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const Vst3PluginProxy::SetState& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": {IComponent,IEditController}::setState(state = "
                   "<IBStream* containing "
                << request.state.size() << "bytes>)";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const Vst3PluginProxy::GetState& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message
            << request.instance_id
            << ": {IComponent,IEditController}::getState(state = <IBStream*>)";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaAudioProcessor::SetBusArrangements& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IAudioProcessor::setBusArrangements(inputs = "
                   "[SpeakerArrangement; "
                << request.inputs.size() << "], numIns = " << request.num_ins
                << ", outputs = [SpeakerArrangement; " << request.outputs.size()
                << "], numOuts = " << request.num_outs << ")";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaAudioProcessor::GetBusArrangement& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IAudioProcessor::getBusArrangement(dir = " << request.dir
                << ", index = " << request.index << ", &arr)";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaAudioProcessor::CanProcessSampleSize& request) {
    return log_request_base(
        is_host_vst, Logger::Verbosity::all_events, [&](auto& message) {
            message
                << request.instance_id
                << ": IAudioProcessor::canProcessSampleSize(symbolicSampleSize "
                   "= "
                << request.symbolic_sample_size << ")";
        });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaAudioProcessor::GetLatencySamples& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IAudioProcessor::getLatencySamples()";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaAudioProcessor::SetupProcessing& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IAudioProcessor::setupProcessing(setup = "
                   "<SetupProcessing with mode = "
                << request.setup.processMode << ", symbolic_sample_size = "
                << request.setup.symbolicSampleSize
                << ", max_buffer_size = " << request.setup.maxSamplesPerBlock
                << " and sample_rate = " << request.setup.sampleRate << ">)";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaAudioProcessor::SetProcessing& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IAudioProcessor::setProcessing(state = "
                << (request.state ? "true" : "false") << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaAudioProcessor::Process& request) {
    return log_request_base(
        is_host_vst, Logger::Verbosity::all_events, [&](auto& message) {
            // This is incredibly verbose, but if you're really a plugin that
            // handles processing in a weird way you're going to need all of
            // this

            std::ostringstream num_input_channels;
            num_input_channels << "[";
            for (bool is_first = true;
                 const auto& buffers : request.data.inputs) {
                num_input_channels << (is_first ? "" : ", ")
                                   << buffers.num_channels();
                is_first = false;
            }
            num_input_channels << "]";

            std::ostringstream num_output_channels;
            num_output_channels << "[";
            for (bool is_first = true;
                 const auto& num_channels : request.data.outputs_num_channels) {
                num_output_channels << (is_first ? "" : ", ") << num_channels;
                is_first = false;
            }
            num_output_channels << "]";

            message << request.instance_id
                    << ": IAudioProcessor::process(data = <ProcessData with "
                       "input_channels = "
                    << num_input_channels.str()
                    << ", output_channels = " << num_output_channels.str()
                    << ", num_samples = " << request.data.process_mode
                    << ", input_parameter_changes = <IParameterChanges* for "
                    << request.data.input_parameter_changes.num_parameters()
                    << " parameters>, output_parameter_changes = "
                    << (request.data.output_parameter_changes_supported
                            ? "<IParameterChanges*>"
                            : "nullptr")
                    << ", input_events = ";
            if (request.data.input_events) {
                message << "<IEventList* with "
                        << request.data.input_events->num_events()
                        << " events>";
            } else {
                message << "nullptr";
            }
            message << ", output_events = "
                    << (request.data.output_events_supported ? "<IEventList*>"
                                                             : "nullptr")
                    << ", process_context = "
                    << (request.data.process_context ? "<ProcessContext*>"
                                                     : "nullptr")
                    << ", process_mode = " << request.data.process_mode
                    << ", symbolic_sample_size = "
                    << request.data.symbolic_sample_size << ">)";
        });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaAudioProcessor::GetTailSamples& request) {
    return log_request_base(
        is_host_vst, Logger::Verbosity::all_events, [&](auto& message) {
            message << request.instance_id
                    << ": IAudioProcessor::getTailSamples()";
        });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponent::SetIoMode& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IComponent::setIoMode(mode = " << request.mode << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponent::GetBusCount& request) {
    // JUCE-based hosts will call this every processing cycle, for some reason
    // (it shouldn't be allwoed to change during processing, right?)
    return log_request_base(
        is_host_vst, Logger::Verbosity::all_events, [&](auto& message) {
            message << request.instance_id
                    << ": IComponent::getBusCount(type = " << request.type
                    << ", dir = " << request.dir << ")";
        });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponent::GetBusInfo& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IComponent::getBusInfo(type = " << request.type
                << ", dir = " << request.dir << ", index = " << request.index
                << ", &bus)";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponent::GetRoutingInfo& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message
            << request.instance_id
            << ": IComponent::getRoutingInfo(inInfo = <RoutingInfo& for bus "
            << request.in_info.busIndex << " and channel "
            << request.in_info.channel << ">, outInfo = <RoutingInfo& for bus "
            << request.out_info.busIndex << " and channel "
            << request.out_info.channel << ">)";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponent::ActivateBus& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IComponent::activateBus(type = " << request.type
                << ", dir = " << request.dir << ", index = " << request.index
                << ", state = " << (request.state ? "true" : "false") << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponent::SetActive& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id << ": IComponent::setActive(state = "
                << (request.state ? "true" : "false") << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaConnectionPoint::Connect& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IConnectionPoint::connect(other = <IConnectionPoint* #"
                << request.other_instance_id << ">)";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaConnectionPoint::Disconnect& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IConnectionPoint::disconnect(other = <IConnectionPoint* #"
                << request.other_instance_id << ">)";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::SetComponentState& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::setComponentState(state = <IBStream* "
                   "containing "
                << request.state.size() << " bytes>)";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::GetParameterCount& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::getParameterCount()";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::GetParameterInfo& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::getParameterInfo(paramIndex = "
                << request.param_index << ", &info)";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::GetParamStringByValue& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::getParamStringByValue(id = "
                << request.id
                << ", valueNormalized = " << request.value_normalized
                << ", &string)";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::GetParamValueByString& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        std::string param_title = VST3::StringConvert::convert(request.string);
        message << request.instance_id
                << ": IEditController::getParamValueByString(id = "
                << request.id << ", string = " << param_title
                << ", &valueNormalized)";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::NormalizedParamToPlain& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::normalizedParamToPlain(id = "
                << request.id
                << ", valueNormalized = " << request.value_normalized << ")";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::PlainParamToNormalized& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::plainParamToNormalized(id = "
                << request.id << ", plainValue = " << request.plain_value
                << ")";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::GetParamNormalized& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::getParamNormalized(id = " << request.id
                << ")";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::SetParamNormalized& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::setParamNormalized(id = " << request.id
                << ", value = " << request.value << ")";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaEditController::SetComponentHandler& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IEditController::setComponentHandler(handler = ";
        if (request.component_handler_proxy_args) {
            message << "<IComponentHandler*>";
        } else {
            message << "<nullptr>";
        }
        message << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaPluginBase::Initialize& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id
                << ": IPluginBase::initialize(context = ";
        if (request.host_context_args) {
            message << "<FUnknown*>";
        } else {
            message << "<nullptr>";
        }
        message << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaPluginBase::Terminate& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.instance_id << ": IPluginBase::terminate()";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaPluginFactory::Construct&) {
    return log_request_base(
        is_host_vst, [&](auto& message) { message << "GetPluginFactory()"; });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaPluginFactory::SetHostContext& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << "IPluginFactory3::setHostContext(";
        if (request.host_context_args) {
            message << "<FUnknown*>";
        } else {
            message << "<nullptr>";
        }
        message << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst, const WantsConfiguration&) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << "Requesting <Configuration>";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponentHandler::BeginEdit& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.owner_instance_id
                << ": IComponentHandler::beginEdit(id = " << request.id << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponentHandler::PerformEdit& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.owner_instance_id
                << ": IComponentHandler::performEdit(id = " << request.id
                << ", valueNormalized = " << request.value_normalized << ")";
    });
}

bool Vst3Logger::log_request(bool is_host_vst,
                             const YaComponentHandler::EndEdit& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.owner_instance_id
                << ": IComponentHandler::endEdit(id = " << request.id << ")";
    });
}

bool Vst3Logger::log_request(
    bool is_host_vst,
    const YaComponentHandler::RestartComponent& request) {
    return log_request_base(is_host_vst, [&](auto& message) {
        message << request.owner_instance_id
                << ": IComponentHandler::restartComponent(flags = "
                << request.flags << ")";
    });
}

void Vst3Logger::log_response(bool is_host_vst, const Ack&) {
    log_response_base(is_host_vst, [&](auto& message) { message << "ACK"; });
}

void Vst3Logger::log_response(bool is_host_vst,
                              const std::variant<Vst3PluginProxy::ConstructArgs,
                                                 UniversalTResult>& result) {
    log_response_base(is_host_vst, [&](auto& message) {
        std::visit(overload{[&](const Vst3PluginProxy::ConstructArgs& args) {
                                message << "<FUnknown* #" << args.instance_id
                                        << ">";
                            },
                            [&](const UniversalTResult& code) {
                                message << code.string();
                            }},
                   result);
    });
}

void Vst3Logger::log_response(
    bool is_host_vst,
    const Vst3PluginProxy::GetStateResponse& response) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << response.result.string();
        if (response.result == Steinberg::kResultOk) {
            message << ", <IBStream* containing "
                    << response.updated_state.size() << " bytes>";
        }
    });
}

void Vst3Logger::log_response(
    bool is_host_vst,
    const YaAudioProcessor::GetBusArrangementResponse& response) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << response.result.string();
        if (response.result == Steinberg::kResultOk) {
            message << ", <SpeakerArrangement>";
        }
    });
}

void Vst3Logger::log_response(
    bool is_host_vst,
    const YaAudioProcessor::ProcessResponse& response) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << response.result.string();

        // This is incredibly verbose, but if you're really a plugin that
        // handles processing in a weird way you're going to need all of this

        std::ostringstream num_output_channels;
        num_output_channels << "[";
        for (bool is_first = true;
             const auto& buffers : response.output_data.outputs) {
            num_output_channels << (is_first ? "" : ", ")
                                << buffers.num_channels();
            is_first = false;
        }
        num_output_channels << "]";

        message << ", <AudioBusBuffers array with " << num_output_channels.str()
                << " channels>";

        if (response.output_data.output_parameter_changes) {
            message << ", <IParameterChanges* for "
                    << response.output_data.output_parameter_changes
                           ->num_parameters()
                    << " parameters>";
        } else {
            message << ", host does not support parameter outputs";
        }

        if (response.output_data.output_events) {
            message << ", <IEventList* with "
                    << response.output_data.output_events->num_events()
                    << " events>";
        } else {
            message << ", host does not support event outputs";
        }
    });
}

void Vst3Logger::log_response(bool is_host_vst,
                              const YaComponent::GetBusInfoResponse& response) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << response.result.string();
        if (response.result == Steinberg::kResultOk) {
            message << ", <BusInfo>";
        }
    });
}

void Vst3Logger::log_response(
    bool is_host_vst,
    const YaComponent::GetRoutingInfoResponse& response) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << response.result.string();
        if (response.result == Steinberg::kResultOk) {
            message << ", <RoutingInfo& for bus "
                    << response.updated_in_info.busIndex << " and channel "
                    << response.updated_in_info.channel
                    << ", <RoutingInfo& for bus "
                    << response.updated_out_info.busIndex << " and channel "
                    << response.updated_out_info.channel << ">";
        }
    });
}

void Vst3Logger::log_response(
    bool is_host_vst,
    const YaEditController::GetParameterInfoResponse& response) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << response.result.string();
        if (response.result == Steinberg::kResultOk) {
            std::string param_title =
                VST3::StringConvert::convert(response.updated_info.title);
            message << ", <ParameterInfo for '" << param_title << "'>";
        }
    });
}

void Vst3Logger::log_response(
    bool is_host_vst,
    const YaEditController::GetParamStringByValueResponse& response) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << response.result.string();
        if (response.result == Steinberg::kResultOk) {
            std::string value = VST3::StringConvert::convert(response.string);
            message << ", \"" << value << "\"";
        }
    });
}

void Vst3Logger::log_response(
    bool is_host_vst,
    const YaEditController::GetParamValueByStringResponse& response) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << response.result.string();
        if (response.result == Steinberg::kResultOk) {
            message << ", " << response.value_normalized << std::endl;
        }
    });
}

void Vst3Logger::log_response(bool is_host_vst,
                              const YaPluginFactory::ConstructArgs& args) {
    log_response_base(is_host_vst, [&](auto& message) {
        message << "<IPluginFactory*> with " << args.num_classes
                << " registered classes";
    });
}

void Vst3Logger::log_response(bool is_host_vst, const Configuration&) {
    log_response_base(is_host_vst,
                      [&](auto& message) { message << "<Configuration>"; });
}
