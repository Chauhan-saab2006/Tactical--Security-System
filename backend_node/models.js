const mongoose = require('mongoose');

// ============================================
// 📊 TABLE STRUCTURE (MONGODB SCHEMA)
// Edit the fields below to add or remove data
// ============================================

const TelemetrySchema = new mongoose.Schema({
    src: { 
        type: String, 
        required: true,
        description: "The hardware Node ID or Wristband ID"
    },
    lat: { 
        type: Number, 
        default: null,
        description: "GPS Latitude Coordinate"
    },
    lng: { 
        type: Number, 
        default: null,
        description: "GPS Longitude Coordinate"
    },
    pulse: { 
        type: Number, 
        default: 0,
        description: "Heart Rate BPM"
    },
    sos: { 
        type: Boolean, 
        default: false,
        description: "Is SOS currently active?"
    },
    rfid: { 
        type: String, 
        default: null,
        description: "Scanned unique RFID tag ID if checkpoint passed"
    },
    timestamp: { 
        type: Date, 
        default: Date.now,
        description: "Exact time packet was received"
    }
});

// Compile the schema into a model accessible in server.js
const Telemetry = mongoose.model('Telemetry', TelemetrySchema);

module.exports = Telemetry;
