let interval = 120;
let frames = [|"-", "\\", "|", "/"|];
/* let frames = [|".  ", ".. ", "..."|]; */

type t = Js.Nullable.t(Js.Global.intervalId);

let n_frames = Array.length(frames);

let start = msg => {
  let i = ref(0);
  Js.Nullable.return(
    Js.Global.setInterval(
      () => {
        Printf.printf("%s %s                                \r", msg, frames[i^ mod n_frames]);
        i := i^ + 1;
      },
      interval,
    ),
  );
};

let cancel = intervalID =>
  Js.Nullable.iter(intervalID, (. intervalId) =>
    Js.Global.clearInterval(intervalId)
  );

let stop = spinnerIntervalID => {
  cancel(spinnerIntervalID);
};

let clearLine = () => {
  Js.log("\n");
};
