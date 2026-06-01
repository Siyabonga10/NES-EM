package com.nesem.app;

import org.json.JSONException;
import org.json.JSONObject;

public class ControlConfig {
    public static class Component {
        public float x; // normalized 0.0 - 1.0
        public float y; // normalized 0.0 - 1.0
        public float scale;

        public Component(float x, float y, float scale) {
            this.x = x;
            this.y = y;
            this.scale = scale;
        }

        public JSONObject toJsonObject() throws JSONException {
            JSONObject obj = new JSONObject();
            obj.put("x", x);
            obj.put("y", y);
            obj.put("scale", scale);
            return obj;
        }

        public static Component fromJsonObject(JSONObject obj) throws JSONException {
            return new Component(
                (float) obj.getDouble("x"),
                (float) obj.getDouble("y"),
                (float) obj.getDouble("scale")
            );
        }
    }

    public Component dpad;
    public Component btnA;
    public Component btnB;
    public Component btnStart;
    public Component btnSelect;

    public ControlConfig() {
        // Defaults will be null, and we'll handle fallback in NesEmView
    }

    public JSONObject toJsonObject() throws JSONException {
        JSONObject obj = new JSONObject();
        if (dpad != null) obj.put("dpad", dpad.toJsonObject());
        if (btnA != null) obj.put("btnA", btnA.toJsonObject());
        if (btnB != null) obj.put("btnB", btnB.toJsonObject());
        if (btnStart != null) obj.put("btnStart", btnStart.toJsonObject());
        if (btnSelect != null) obj.put("btnSelect", btnSelect.toJsonObject());
        return obj;
    }

    public static ControlConfig fromJson(String json) {
        try {
            JSONObject obj = new JSONObject(json);
            ControlConfig config = new ControlConfig();
            if (obj.has("dpad")) config.dpad = Component.fromJsonObject(obj.getJSONObject("dpad"));
            if (obj.has("btnA")) config.btnA = Component.fromJsonObject(obj.getJSONObject("btnA"));
            if (obj.has("btnB")) config.btnB = Component.fromJsonObject(obj.getJSONObject("btnB"));
            if (obj.has("btnStart")) config.btnStart = Component.fromJsonObject(obj.getJSONObject("btnStart"));
            if (obj.has("btnSelect")) config.btnSelect = Component.fromJsonObject(obj.getJSONObject("btnSelect"));
            return config;
        } catch (Exception e) {
            return null;
        }
    }
}
