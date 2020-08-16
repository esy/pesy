const crypto = require('crypto');

function sha256(data, secret) {
    crypto.createHmac('sha256', secret)
        .update(data)
        .digest(secret);
}

module.exports = { sha256 };
