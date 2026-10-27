const crypto = require('crypto');

// Must match common_utils.h strictly
const MIDWAY_MASTER_KEY = "MIDWAY_MASTER_KEY_00000000000000";
const STANDARD_IV = "ivsecretivsecret";

function decryptMidwayPayload(b64Text) {
    try {
        const decipher = crypto.createDecipheriv(
            'aes-256-cbc',
            Buffer.from(MIDWAY_MASTER_KEY, 'utf-8'),
            Buffer.from(STANDARD_IV, 'utf-8')
        );

        // Turn auto-padding off or keep it on if standard PKCS7 is used by Arduino
        decipher.setAutoPadding(true); 

        let decrypted = decipher.update(b64Text, 'base64', 'utf8');
        decrypted += decipher.final('utf8');

        // Remove possible manual padding if mbedtls padded it heavily
        decrypted = decrypted.replace(/[\x00-\x1F]+/g, ''); 
        
        return JSON.parse(decrypted);
    } catch (e) {
        console.error("AES Decryption Error:", e.message);
        return null;
    }
}

module.exports = { decryptMidwayPayload };
