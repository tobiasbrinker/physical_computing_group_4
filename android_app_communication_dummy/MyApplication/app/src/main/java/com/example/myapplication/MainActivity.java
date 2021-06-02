package com.example.myapplication;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import com.android.volley.toolbox.Volley;
import com.android.volley.RequestQueue;
import com.android.volley.Request;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.StringRequest;

import java.util.HashMap;
import java.util.Map;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        String server ="https://webhook.site/7816a41d-3184-415a-98f5-f9e6842d31b8"; // https://webhook.site easy testing
        String msg = "Event1-Tobi";

        send(server, msg);
        receive(server);
    }

    public void sendMessage_using_button(View view) {
        String server ="https://webhook.site/7816a41d-3184-415a-98f5-f9e6842d31b8"; // https://webhook.site easy testing
        String msg = "Event1-Tobi";
        send(server, msg);
    }

    void send(String server, String msg) {
        RequestQueue requestQueue = Volley.newRequestQueue(this);
        final String requestBody = msg;

        final TextView textView = (TextView) findViewById(R.id.textView);
        textView.setText(msg);

        StringRequest MyStringRequest = new StringRequest(Request.Method.POST, server, new Response.Listener<String>() {
            @Override
            public void onResponse(String response) {
                //This code is executed if the server responds, whether or not the response contains data.
                //The String 'response' contains the server's response.
            }
        }, new Response.ErrorListener() { //Create an error listener to handle errors appropriately.
            @Override
            public void onErrorResponse(VolleyError error) {
                //This code is executed if there is an error.
            }
        }) {
            protected Map<String, String> getParams() {
                Map<String, String> MyData = new HashMap<String, String>();
                MyData.put("Msg:", msg);
                return MyData;
            }
        };

        requestQueue.add(MyStringRequest);
    }

    void receive(String server) {
        final TextView textView = (TextView) findViewById(R.id.responseView);

        RequestQueue queue = Volley.newRequestQueue(this);

        StringRequest stringRequest = new StringRequest(Request.Method.GET, server,
                new Response.Listener<String>() {
                    @Override
                    public void onResponse(String response) {
                        // Display the first 500 characters of the response string.
                        textView.setText(response);
                    }
                }, new Response.ErrorListener() {
            @Override
            public void onErrorResponse(VolleyError error) {
                textView.setText("That didn't work!");
            }
        });

        queue.add(stringRequest);
    }
}