#include "opentelemetry/trace/context.h"

#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_context_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/provider.h"

#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <opentelemetry/trace/span.h>
#include <thread>

#include "App.h"

// void InitTracer() {
//   auto exporter =
//       opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create();
//   auto processor =
//       opentelemetry::sdk::trace::SimpleSpanProcessorFactory::Create(
//           std::move(exporter));
//   std::vector<std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>>
//       processors;
//   processors.push_back(std::move(processor));
//   // Default is an always-on sampler.
//   std::shared_ptr<opentelemetry::sdk::trace::TracerContext> context =
//       opentelemetry::sdk::trace::TracerContextFactory::Create(
//           std::move(processors));
//   std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
//       opentelemetry::sdk::trace::TracerProviderFactory::Create(context);
//   // Set the global trace provider
//   opentelemetry::trace::Provider::SetTracerProvider(provider);

//   // set global propagator
//   opentelemetry::context::propagation::GlobalTextMapPropagator::
//       SetGlobalPropagator(
//           opentelemetry::nostd::shared_ptr<
//               opentelemetry::context::propagation::TextMapPropagator>(
//               new opentelemetry::trace::propagation::HttpTraceContext()));
// }

void InitTracer() {
  namespace trace_exporter = opentelemetry::exporter::trace;
  namespace trace_api = opentelemetry::trace;
  namespace trace_sdk = opentelemetry::sdk::trace;

  // setup exporter
  auto exporter = trace_exporter::OStreamSpanExporterFactory::Create();

  // setup processor
  auto processor =
      trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));

  // setup tracer provider
  std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
      trace_sdk::TracerProviderFactory::Create(std::move(processor));

  // Set the global trace provider
  trace_api::Provider::SetTracerProvider(provider);
}

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>
get_tracer(std::string tracer_name) {
  auto provider = opentelemetry::trace::Provider::GetTracerProvider();
  return provider->GetTracer(tracer_name, "0.0.1");
}

thread_local static opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
    _span;

void set_current_span(
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> &span) {
  _span = span;
}

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
get_current_span() {
  return _span;
}

void CleanupTracer() {
  std::shared_ptr<opentelemetry::trace::TracerProvider> none;
  trace_api::Provider::SetTracerProvider(none);
}

int main() {
  InitTracer();

  auto root_span = get_tracer("http_server")->StartSpan(__func__);
  opentelemetry::trace::Scope scope(root_span);

  /* Overly simple hello world app */
  uWS::App()
      .onHttpStart([&scope]() {
        std::cout << "onHttpStart" << std::endl;

        opentelemetry::trace::StartSpanOptions options;
        options.kind = opentelemetry::trace::SpanKind::kServer; // server
        std::string span_name = "request.uri";

        auto tracer = get_tracer("http-server");

        auto span = tracer->StartSpan(span_name, options);
        // tracer->WithActiveSpan(span); // dont, when scope ends, the trace
        // stops

        set_current_span(span);
      })
      .onHttpEnd([]() {
        std::cout << "onHttpEnd" << std::endl;
        auto t = 1;

        get_current_span()->End();
      })
      .onHttpException([](std::exception_ptr e) {
        std::cout << "onHttpException" << std::endl;
        try {
          std::rethrow_exception(e);
        } catch (const std::exception &e) {
          std::cout << "Server caught:\n" << e.what() << std::endl;
        }

        get_current_span()->End();
      })
      .get("/exception",
           [](auto *, auto *) {
             throw std::runtime_error("Exception from handler");
           })
      .get("/*",
           [](auto *res, auto * /*req*/) {
             auto foo_lib_tracer = get_tracer("foo_lib");
             auto outer_span = foo_lib_tracer->StartSpan("reading_foo_list");
             foo_lib_tracer->WithActiveSpan(outer_span);

             std::this_thread::sleep_for(std::chrono::milliseconds(200));

             {
               auto inner_span = foo_lib_tracer->StartSpan("updating_bar_list");
               foo_lib_tracer->WithActiveSpan(inner_span);

               std::this_thread::sleep_for(std::chrono::milliseconds(300));

               inner_span->End();
             }

             outer_span->End();

             res->end("Hello world!");
           })
      .listen(3512,
              [](auto *listen_socket) {
                if (listen_socket) {
                  std::cout << "Listening on port " << 3512 << std::endl;
                }
              })
      .run();

  CleanupTracer();

  std::cout << "Failed to listen on port 3512" << std::endl;
}
