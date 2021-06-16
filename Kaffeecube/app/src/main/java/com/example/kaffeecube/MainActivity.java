package com.example.kaffeecube;

/*
https://couchdb.hci.uni-hannover.de/_utils/#/database/s21-pcl-g4/_all_docs
https://couchdb.hci.uni-hannover.de/s21-pcl-g4/activities
curl -X PUT https://couchdb.hci.uni-hannover.de/s21-pcl-g4/doc124 -d '{"key":"my key","value":"my value"}'
*/

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.atomic.AtomicReference;

public class MainActivity extends AppCompatActivity {

    public final static String dbUrl = "https://couchdb.hci.uni-hannover.de/s21-pcl-g4/";
    public final static String initialDocId = "activities";
    private TextView tvDocId, tvDoc, tvResponse;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // text views
        tvDocId = (TextView) findViewById(R.id.editTextDocId);
        tvDoc = (TextView) findViewById(R.id.editTextDoc);
        tvResponse = (TextView) findViewById(R.id.dbResponse);
        tvDocId.setText(initialDocId);

        // register get-button click
        Button getButton = (Button) findViewById(R.id.buttonGet);
        getButton.setOnClickListener((View view) -> {
            // get document from server
            String docId = tvDocId.getText().toString();
            dbGet(dbUrl + docId, (JSONObject doc) -> {
                try {
                    tvDoc.setText(doc.toString(2));
                    tvResponse.setText("get ok");
                    final JSONObject testDoc = doc;
                } catch (JSONException ex) {
                    Log.d("MainActivity", ex.toString());
                }
            }, (Exception ex) -> {
                // show message if there was a problem
                tvResponse.setText(ex.toString());
            });
        });

        // register set-button click
        Button setButton = (Button) findViewById(R.id.buttonSet);
        setButton.setOnClickListener((View view) -> {
            try {
                String docId = tvDocId.getText().toString();
                String doc = tvDoc.getText().toString();
                JSONObject jobj = new JSONObject(doc);
                // send document to server (and show response from server)
                dbPut(dbUrl + docId, jobj, (JSONObject response) -> {
                    tvResponse.setText(response.toString());
                }, null);
            } catch (JSONException ex) {
                Log.d("MainActivity", ex.toString());
            }
        });

        Button triggerButton = (Button) findViewById(R.id.trigger_button);
        triggerButton.setOnClickListener((View view) -> {
            System.out.print("BUTTON TRIGGERED");
            setDetails("sport", "Oli,Tobi,Kolja", tvDocId.getText().toString());
        });
    }

    public void setDetails(String activity, String participants, String docId) {
        dbGet(dbUrl + docId, (JSONObject doc) -> {
            try {
                tvDoc.setText(doc.toString(2));
            } catch (JSONException ex) {
                Log.d("MainActivity", ex.toString());
            }
        }, (Exception ex) -> {
            // show message if there was a problem
            tvResponse.setText(ex.toString());
        });

        try {
            String doc = tvDoc.getText().toString();
            JSONObject jobj = new JSONObject(doc);
            jobj.put("participant", participants);
            jobj.put("activity", activity);
            // send document to server (and show response from server)
            dbPut(dbUrl + docId, jobj, (JSONObject response) -> {
                tvResponse.setText(response.toString());
            }, null);
        } catch (JSONException ex) {
            Log.d("MainActivity", ex.toString());
        }
    }

    /**
     * Asynchronously get a JSON document from the database server.
     * @param urlString The URL of the server and document, e.g., "https://server.org/database/doc123"
     * @param jsonHandler Called to handle the server's response.
     * @param failureHandler Called when there was a problem. The response is an Exception object.
     *                       Set to null if no error processing is required.
     */
     public void dbGet(String urlString, JSONHandler jsonHandler, FailureHandler failureHandler) {
        assert urlString != null && urlString.startsWith("http");
        new Thread(() -> {
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
                    runOnUiThread(() -> {
                        jsonHandler.handle(jobj);
                    });
                }
            } catch (Exception ex) {
                Log.d("MainActivity", ex.toString());
                if (failureHandler != null) {
                    runOnUiThread(() -> {
                        failureHandler.handle(ex);
                    });
                }
            } finally {
                if (con != null) con.disconnect();
            }
        }).start();
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
    public void dbPut(String urlString, JSONObject doc, JSONHandler jsonHandler, FailureHandler failureHandler) {
        assert urlString != null && urlString.startsWith("http");
        assert doc != null;
        new Thread(() -> {
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
                    runOnUiThread(() -> {
                        jsonHandler.handle(jobj);
                    });
                }
            } catch (Exception ex) {
                Log.d("MainActivity", ex.toString());
                if (failureHandler != null) {
                    runOnUiThread(() -> {
                        failureHandler.handle(ex);
                    });
                }
            } finally {
                if (con != null) con.disconnect();
            }
        }).start();
    }

}