open ReActor;
open ReActor_Utils;
open FFI_Runtime;

type sample = {counter: int};

let noop: Process.f(sample) =
  (env, state) => {
    if (state.counter == 2112) {
      let p = env.self() |> Pid.toString;
      Js.log({j|$p got to 2112|j});
    };
    env.loop({counter: state.counter + 1});
  };

let loop_noop: Process.f(sample) =
  (env, state) => {
    env.sleep(
      100,
      () => {
        let now = Date.now();
        let counter = state.counter;
        let pid = Pid.toString(env.self());
        Js.log({j|$now - $pid - $counter|j});
        env.loop({counter: state.counter - 1});
      },
    );
    state;
  };

let named_slower_pid =
  spawn(loop_noop, {counter: 0}) |> register("slower_pid");
let anon_slower_pid = spawn(loop_noop, {counter: 0});

let pids: list(Pid.t) =
  Array.make(10000, 0)
  |> Array.map(i => spawn(noop, {counter: i}))
  |> Array.to_list;

defer(
  () => {
    let p = anon_slower_pid |> Pid.toString;
    Js.log({j|Anon Killing $p by pid|j});
    exit(anon_slower_pid);
  },
  1500,
);
defer(
  () =>
    switch (where_is("slower_pid")) {
    | None => Js.log("Could not find pid \"slower_pid\"")
    | Some(pid) =>
      let p = pid |> Pid.toString;
      Js.log({j|Killing $p by name "slower_pid"|j});
      exit(pid);
    },
  3000,
);