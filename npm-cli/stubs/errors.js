module.exports.ExecError = class ExecError extends Error {
  constructor(cmd, stdout, stderr) {
    super(`ExecError: cmd: ${cmd}\nstdout: ${stdout}\nstderr: ${stderr}`);
    this.cmd = cmd;
    this.stdout = stdout;
    this.stderr = stderr;
  }
};
