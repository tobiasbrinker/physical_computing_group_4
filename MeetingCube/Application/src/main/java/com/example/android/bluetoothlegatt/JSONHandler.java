package com.example.android.bluetoothlegatt;

import org.json.JSONException;
import org.json.JSONObject;

public interface JSONHandler {
    public void handle(JSONObject jobj) throws JSONException;
}
