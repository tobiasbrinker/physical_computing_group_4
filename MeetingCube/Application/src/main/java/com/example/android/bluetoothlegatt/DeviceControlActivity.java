/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.bluetoothlegatt;

import android.app.Activity;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ExpandableListView;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TextView;




import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.atomic.AtomicReference;

/**
 * For a given BLE device, this Activity provides the user interface to connect, display data,
 * and display GATT services and characteristics supported by the device.  The Activity
 * communicates with {@code BluetoothLeService}, which in turn interacts with the
 * Bluetooth LE API.
 */
public class DeviceControlActivity extends Activity {
    private final static String TAG = DeviceControlActivity.class.getSimpleName();

    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    // TOBIS DATA
    public static TextView tvDoc, tvResponse;
    public static TextView tvDocId;
    public final static String dbUrl = "https://couchdb.hci.uni-hannover.de/s21-pcl-g4/";
    public final static String initialDocId = "activities";

    private TextView mConnectionState;
    private TextView mDataField;
    private String mDeviceName;
    private String mDeviceAddress;
    private ExpandableListView mGattServicesList;
    private BluetoothLeService mBluetoothLeService;
    private ArrayList<ArrayList<BluetoothGattCharacteristic>> mGattCharacteristics =
            new ArrayList<ArrayList<BluetoothGattCharacteristic>>();
    private boolean mConnected = false;
    private BluetoothGattCharacteristic mNotifyCharacteristic;

    private final String LIST_NAME = "NAME";
    private final String LIST_UUID = "UUID";

    private String signalOneRecieve = "1";

    // Code to manage Service lifecycle.
    private final ServiceConnection mServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName componentName, IBinder service) {
            mBluetoothLeService = ((BluetoothLeService.LocalBinder) service).getService();
            if (!mBluetoothLeService.initialize()) {
                Log.e(TAG, "Unable to initialize Bluetooth");
                finish();
            }
            // Automatically connects to the device upon successful start-up initialization.
            mBluetoothLeService.connect(mDeviceAddress);

            Timer timer = new Timer("DBUpdateTimer");
            TimerTask task = new TimerTask() {
                @Override
                public void run() {
                    System.out.println("Getting DB");
                    dbGet(dbUrl + tvDocId.getText().toString(), (JSONObject doc) -> {
                        try {
                            String response = doc.toString(2);
                            int activity = checkActivities(response);
                            byte[] value = new byte[1];
                            //TODO change int
                            switch(activity) {
                                case 1:
                                    //TODO change button to real sensor
                                    System.out.println("LED AN (BUTTON)");
                                    value[0] = (byte) (0x04);
                                    mBluetoothLeService.writeCustomCharacteristic(value);
                                    break;
                                case 2:
                                    System.out.println("LED AN (HEARBEAT)");
                                    value[0] = (byte) (0x05);
                                    mBluetoothLeService.writeCustomCharacteristic(value);
                                    break;
                                default:
                                    System.out.println("LED AUS");
                                    value[0] = (byte) (0x00);
                                    mBluetoothLeService.writeCustomCharacteristic(value);
                                    break;
                            }
                        } catch (JSONException ex) {
                            Log.d("Timer", ex.toString());
                        }
                    }, (Exception ex) -> {
                        // show message if there was a problem
                        System.out.println(ex.toString());

                    });
                }
            };
            timer.scheduleAtFixedRate(task, 0, 2000);

            Timer timer_resetButton = new Timer("resetButton");
            TimerTask reset_task = new TimerTask() {
                @Override
                public void run() {
                    BluetoothLeService.eventOneExecuted = false;
                    BluetoothLeService.eventTwoExecuted = false;
                }
            };
            timer_resetButton.scheduleAtFixedRate(reset_task, 0, 10000);
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBluetoothLeService = null;
        }
    };

    // Handles various events fired by the Service.
    // ACTION_GATT_CONNECTED: connected to a GATT server.
    // ACTION_GATT_DISCONNECTED: disconnected from a GATT server.
    // ACTION_GATT_SERVICES_DISCOVERED: discovered GATT services.
    // ACTION_DATA_AVAILABLE: received data from the device.  This can be a result of read
    //                        or notification operations.
    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BluetoothLeService.ACTION_GATT_CONNECTED.equals(action)) {
                mConnected = true;
                updateConnectionState(R.string.connected);
                invalidateOptionsMenu();
            } else if (BluetoothLeService.ACTION_GATT_DISCONNECTED.equals(action)) {
                mConnected = false;
                updateConnectionState(R.string.disconnected);
                invalidateOptionsMenu();
                clearUI();
            } else if (BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED.equals(action)) {
                // Show all the supported services and characteristics on the user interface.
                //displayGattServices(mBluetoothLeService.getSupportedGattServices());
            } else if (BluetoothLeService.ACTION_DATA_AVAILABLE.equals(action)) {
                displayData(intent.getStringExtra(BluetoothLeService.EXTRA_DATA));
            }
        }
    };

    // If a given GATT characteristic is selected, check for supported features.  This sample
    // demonstrates 'Read' and 'Notify' features.  See
    // http://d.android.com/reference/android/bluetooth/BluetoothGatt.html for the complete
    // list of supported characteristic features.
    private final ExpandableListView.OnChildClickListener servicesListClickListner =
            new ExpandableListView.OnChildClickListener() {
                @Override
                public boolean onChildClick(ExpandableListView parent, View v, int groupPosition,
                                            int childPosition, long id) {
                    if (mGattCharacteristics != null) {
                        final BluetoothGattCharacteristic characteristic =
                                mGattCharacteristics.get(groupPosition).get(childPosition);
                        final int charaProp = characteristic.getProperties();
                        if ((charaProp | BluetoothGattCharacteristic.PROPERTY_READ) > 0) {
                            // If there is an active notification on a characteristic, clear
                            // it first so it doesn't update the data field on the user interface.
                            if (mNotifyCharacteristic != null) {
                                mBluetoothLeService.setCharacteristicNotification(
                                        mNotifyCharacteristic, false);
                                mNotifyCharacteristic = null;
                            }
                            mBluetoothLeService.readCharacteristic(characteristic);
                        }
                        if ((charaProp | BluetoothGattCharacteristic.PROPERTY_NOTIFY) > 0) {
                            mNotifyCharacteristic = characteristic;
                            mBluetoothLeService.setCharacteristicNotification(
                                    characteristic, true);
                        }
                        return true;
                    }
                    return false;
                }
    };

    private void clearUI() {
        //mGattServicesList.setAdapter((SimpleExpandableListAdapter) null);
        mDataField.setText(R.string.no_data);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.button_control);

        final Intent intent = getIntent();
        mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
        mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

        // text views
        tvDocId = (TextView) findViewById(R.id.editTextDocId);
        tvDoc = (TextView) findViewById(R.id.editTextDoc);
        tvResponse = (TextView) findViewById(R.id.dbResponse);
        tvDocId.setText(initialDocId);


        mDataField = (TextView) findViewById(R.id.data_value);

        getActionBar().setTitle(mDeviceName);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        Intent gattServiceIntent = new Intent(this, BluetoothLeService.class);
        bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE);
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerReceiver(mGattUpdateReceiver, makeGattUpdateIntentFilter());
        if (mBluetoothLeService != null) {
            final boolean result = mBluetoothLeService.connect(mDeviceAddress);
            Log.d(TAG, "Connect request result=" + result);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mGattUpdateReceiver);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unbindService(mServiceConnection);
        mBluetoothLeService = null;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.gatt_services, menu);
        if (mConnected) {
            menu.findItem(R.id.menu_connect).setVisible(false);
            menu.findItem(R.id.menu_disconnect).setVisible(true);
        } else {
            menu.findItem(R.id.menu_connect).setVisible(true);
            menu.findItem(R.id.menu_disconnect).setVisible(false);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case R.id.menu_connect:
                mBluetoothLeService.connect(mDeviceAddress);
                return true;
            case R.id.menu_disconnect:
                mBluetoothLeService.disconnect();
                return true;
            case android.R.id.home:
                onBackPressed();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void updateConnectionState(final int resourceId) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                //mConnectionState.setText(resourceId);
            }
        });
    }

    private void displayData(String data) {
        if (data != null) {
            mDataField.setText(data);
        }
    }

    // Demonstrates how to iterate through the supported GATT Services/Characteristics.
    // In this sample, we populate the data structure that is bound to the ExpandableListView
    // on the UI.
    private void displayGattServices(List<BluetoothGattService> gattServices) {
        if (gattServices == null) return;
        String uuid;
        String unknownServiceString = getResources().getString(R.string.unknown_service);
        String unknownCharaString = getResources().getString(R.string.unknown_characteristic);
        ArrayList<HashMap<String, String>> gattServiceData = new ArrayList<>();
        ArrayList<ArrayList<HashMap<String, String>>> gattCharacteristicData
                = new ArrayList<>();
        mGattCharacteristics = new ArrayList<>();

        // Loops through available GATT Services.
        for (BluetoothGattService gattService : gattServices) {
            HashMap<String, String> currentServiceData = new HashMap<>();
            uuid = gattService.getUuid().toString();
            currentServiceData.put(
                    LIST_NAME, SampleGattAttributes.lookup(uuid, unknownServiceString));
            currentServiceData.put(LIST_UUID, uuid);
            gattServiceData.add(currentServiceData);

            ArrayList<HashMap<String, String>> gattCharacteristicGroupData =
                    new ArrayList<HashMap<String, String>>();
            List<BluetoothGattCharacteristic> gattCharacteristics =
                    gattService.getCharacteristics();
            ArrayList<BluetoothGattCharacteristic> charas =
                    new ArrayList<BluetoothGattCharacteristic>();

            // Loops through available Characteristics.
            for (BluetoothGattCharacteristic gattCharacteristic : gattCharacteristics) {
                charas.add(gattCharacteristic);
                HashMap<String, String> currentCharaData = new HashMap<String, String>();
                uuid = gattCharacteristic.getUuid().toString();
                currentCharaData.put(
                        LIST_NAME, SampleGattAttributes.lookup(uuid, unknownCharaString));
                currentCharaData.put(LIST_UUID, uuid);
                gattCharacteristicGroupData.add(currentCharaData);
            }
            mGattCharacteristics.add(charas);
            gattCharacteristicData.add(gattCharacteristicGroupData);
        }

        SimpleExpandableListAdapter gattServiceAdapter = new SimpleExpandableListAdapter(
                this,
                gattServiceData,
                android.R.layout.simple_expandable_list_item_2,
                new String[] {LIST_NAME, LIST_UUID},
                new int[] { android.R.id.text1, android.R.id.text2 },
                gattCharacteristicData,
                android.R.layout.simple_expandable_list_item_2,
                new String[] {LIST_NAME, LIST_UUID},
                new int[] { android.R.id.text1, android.R.id.text2 }
        );
        mGattServicesList.setAdapter(gattServiceAdapter);
    }

    private static IntentFilter makeGattUpdateIntentFilter() {
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_CONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(BluetoothLeService.ACTION_DATA_AVAILABLE);
        return intentFilter;
    }
    public void onClickWrite(View v){
        if(mBluetoothLeService != null) {
            byte[] value = new byte[1];
            value[0] = (byte) (0x04);
            mBluetoothLeService.writeCustomCharacteristic(value);
        }
    }

    public void onClickRead(View v){
        if(mBluetoothLeService != null) {
            byte[] value = new byte[1];
            value[0] = (byte) (0x00);
            mBluetoothLeService.writeCustomCharacteristic(value);

        }
    }

    public static void addToActivity(String docId, String userName, Integer activity) {
        System.out.println("Adding to Actitity!");
        try {
            final String[] json_data = {""};
            dbGet(dbUrl + docId, (JSONObject doc) -> {
                json_data[0] = doc.toString(2);
            }, (Exception ex) -> {
                // show message if there was a problem
                Log.d("addToActivitydbget", ex.toString());
            });

            String updated_participants = "";
            String doc = json_data[0];
            if (doc != "") {
                JSONObject jobj = new JSONObject(doc);
                String participants = jobj.get("participant").toString();
                if (participants == "") {
                    updated_participants = new StringBuilder().append(participants).append(userName).toString();
                } else {
                    updated_participants = new StringBuilder().append(participants).append(",").append(userName).toString();
                }
                final String participants_final = updated_participants;
                jobj.put("participant", participants_final);
                jobj.put("activity", activity);
                // send document to server (and show response from server)
                dbPut(dbUrl + docId, jobj, (JSONObject response) -> {
                    Log.d("dbPut:", participants_final);
                }, null);
            } else {
                Log.d("addToActivityJSON", "Could not get JSON Data!");
            }
        } catch (JSONException ex) {
            Log.d("addToActivitydbPut", ex.toString());
        }
    }

    /**
     * Asynchronously get a JSON document from the database server.
     * @param urlString The URL of the server and document, e.g., "https://server.org/database/doc123"
     * @param jsonHandler Called to handle the server's response.
     * @param failureHandler Called when there was a problem. The response is an Exception object.
     *                       Set to null if no error processing is required.
     */
    public static void dbGet(String urlString, JSONHandler jsonHandler, FailureHandler failureHandler) {
        assert urlString != null && urlString.startsWith("http");
            HttpURLConnection con = null;
            try {
                URL url = new URL(urlString);
                con = (HttpURLConnection) url.openConnection();
                String json = "";
                BufferedReader in = new BufferedReader(new InputStreamReader(con.getInputStream()));
                String s = null;
                while ((s = in.readLine()) != null) {
                    json += s;
                }
                in.close();
                con.disconnect();
                con = null;
                if (jsonHandler != null) {
                    JSONObject jobj = new JSONObject(json);
                    jsonHandler.handle(jobj);
                }
            } catch (Exception ex) {
                Log.d("dbGet", ex.toString());
                if (failureHandler != null) {

                    failureHandler.handle(ex);
                }
            } finally {
                if (con != null) con.disconnect();
            }
    }

    /**
     * Asynchronously send/update a JSON document on the database server. To update a document, its
     * current revision number ("_rev") must be given in the document.
     * @param urlString The URL of the server and document, e.g., "https://server.org/database/doc123"
     * @param doc The JSON document to set/update.
     * @param jsonHandler Called to handle the server's response.
     * @param failureHandler Called when there was a problem. The response is an Exception object.
     *                       Set to null if no error processing is required.
     */
    public static void dbPut(String urlString, JSONObject doc, JSONHandler jsonHandler, FailureHandler failureHandler) {
        assert urlString != null && urlString.startsWith("http");
        assert doc != null;
            HttpURLConnection con = null;
            try {
                URL url = new URL(urlString);
                con = (HttpURLConnection) url.openConnection();
                con.setDoOutput(true);
                con.setRequestMethod("PUT");
                con.setChunkedStreamingMode(0); // write in chunks
                con.setRequestProperty("Content-type", "application/json");
                con.setRequestProperty("Accept", "application/json");
                BufferedWriter out = new BufferedWriter(new OutputStreamWriter(con.getOutputStream()));
                out.write(doc.toString());
                out.close();
                BufferedReader in = new BufferedReader(new InputStreamReader(con.getInputStream()));
                String json = "";
                String s = null;
                while ((s = in.readLine()) != null) {
                    json += s;
                }
                in.close();
                con.disconnect();
                con = null;
                if (jsonHandler != null) {
                    JSONObject jobj = new JSONObject(json);
                        jsonHandler.handle(jobj);

                }
            } catch (Exception ex) {
                Log.d("dbPut", ex.toString());
                if (failureHandler != null) {
                        failureHandler.handle(ex);
                }
            } finally {
                if (con != null) con.disconnect();
            }
    }
    private int checkActivities(String response) {
        int activity = 0;
        try {
            JSONObject jobj = new JSONObject(response);
            activity = Integer.parseInt(jobj.get("activity").toString());
        } catch (JSONException e) {
            Log.d("checkActivities", e.toString());
        }
        return activity;

    }
}
