type t = {
  namespace: string,
  dunePublicName: string,
};

let getNamespace = pm => pm.namespace;
let getDunePublicName = pm => pm.dunePublicName;
let create: (string, string) => t =
  (namespace, dunePublicName) => {
    {namespace, dunePublicName};
  };
