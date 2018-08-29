open ReActor;
open ReActor_Utils;
open FFI_Runtime;

module DOMLogger = {
  let __name = "logger";

  type state = {node: DOM.node};

  type Message.t +=
    | Log(string);

  let logger_f: Process.f(state) =
    (env, state) => {
      env.recv(
        fun
        | Log(msg) => {
            DOM.withInnerText(state.node, msg);
            env.loop(state);
          }
        | _ => env.loop(state),
      );
      state;
    };

  let dom_logger =
    spawn(logger_f, {node: DOM.getElementById("sample")})
    |> register(__name);

  let logInt: int => Message.t =
    s => Log(string_of_int(s) ++ "ms - " ++ Random.shortId());
};

module Differ = {
  type config = {
    send_to: Pid.t,
    wrap: int => Message.t,
  };

  type Message.t +=
    | Diff(int);

  let f: Process.f(config) =
    (env, config) => {
      env.recv(
        fun
        | Diff(t) => {
            let delta = Performance.now() - t;
            send(config.send_to, config.wrap(delta));
            env.loop(config);
          }
        | _ => env.loop(config),
      );
      config;
    };

  let start = spawn(f);
};

module Clock = {
  type config = {
    delay: int,
    send_to: Pid.t,
    wrap: int => Message.t,
  };
  let clock_f: Process.f(config) =
    (env, config) => {
      env.sleep(
        config.delay,
        () => {
          send(config.send_to, config.wrap(Performance.now()));
          env.loop(config);
        },
      );
      config;
    };

  let start = spawn(clock_f);
};

switch (where_is("logger")) {
| Some(pid) =>
  let differ = Differ.start({send_to: pid, wrap: DOMLogger.logInt});
  let _clock =
    Clock.start({delay: 0, send_to: differ, wrap: x => Differ.Diff(x)});
  ();
| None => Js.log("Failed to start logger.")
};
