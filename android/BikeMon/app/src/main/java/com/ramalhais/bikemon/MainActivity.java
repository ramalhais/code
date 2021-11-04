package com.ramalhais.bikemon;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Paint;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.text.Layout;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.AxisBase;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.formatter.IAxisValueFormatter;
import com.github.mikephil.charting.formatter.ValueFormatter;
import com.jjoe64.graphview.DefaultLabelFormatter;
import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.LegendRenderer;
import com.jjoe64.graphview.ValueDependentColor;
import com.jjoe64.graphview.helper.DateAsXAxisLabelFormatter;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.text.SimpleDateFormat;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import static java.lang.Math.abs;

public class MainActivity extends AppCompatActivity implements SensorEventListener, LocationListener {
    private static final String TAG = "MainActivity";

    private boolean isBletoothConnected = false;
    private UUID btUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int REQUEST_ENABLE_BT = 1;
    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSocket mSocket;
    private BufferedReader btInput;
    private BufferedWriter btOutput;


    private SensorManager sensorManager;
    private Sensor accelerometer;
    private LocationManager lm;
    private int mCount = 0;

    private GraphView graph;
    private LineGraphSeries<DataPoint> mSeriesAFR;
    private LineGraphSeries<DataPoint> mSeriesAccel;
    private LineGraphSeries<DataPoint> mSeriesSpeed;

    private List<Entry> entriesAFR = new ArrayList<Entry>();
    private List<Entry> entriesAccel = new ArrayList<Entry>();
    private List<Entry> entriesSpeed = new ArrayList<Entry>();
    private LineChart chart;

    private final BroadcastReceiver receiver = new BroadcastReceiver() {
//        @RequiresApi(api = Build.VERSION_CODES.KITKAT)
        @RequiresApi(api = Build.VERSION_CODES.KITKAT)
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (!isBletoothConnected && BluetoothDevice.ACTION_FOUND.equals(action)) {
                // Discovery has found a device. Get the BluetoothDevice
                // object and its info from the Intent.
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (device.getName() != null) {
                    Log.v(TAG, "Discovered Bluetooth " + device.getName());
                    if (device.getName().compareTo("BikeMon") == 0) {
                        int bondState = device.getBondState();
                        if (bondState != BluetoothDevice.BOND_BONDING
                            && bondState != BluetoothDevice.BOND_BONDED) {
                            Log.v(TAG, "Bluetooth bonding to " + device.getName());
                            device.createBond(); // Requires kitkat
                        }
//                        String deviceHardwareAddress = device.getAddress(); // MAC address
                        //                    device.setPairingConfirmation(true); // requires kitkat
                        try {
                            Log.v(TAG, "Connecting to Bluetooth " + device.getName());
                            mSocket = device.createRfcommSocketToServiceRecord(btUUID);
                            mSocket.connect();
                            btInput = new BufferedReader(new InputStreamReader(mSocket.getInputStream()));
                            btOutput = new BufferedWriter(new OutputStreamWriter(mSocket.getOutputStream()));
                            isBletoothConnected = true;
                        } catch (IOException e) {
                            isBletoothConnected = false;
                            Log.v(TAG, "Connection failed to Bluetooth " + device.getName());
                            e.printStackTrace();
                        }
                    }
                }
            }
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
        sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_NORMAL);
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED && ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        lm.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        sensorManager.unregisterListener(this);
        lm.removeUpdates(this);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        graph = new GraphView(this);
//        graph.getViewport().setScalableY(true);
//        graph.getViewport().setScalable(true);
        graph.getViewport().setXAxisBoundsManual(true);
        graph.getViewport().setMinX(4);
        graph.getViewport().setMaxX(200);
        graph.getLegendRenderer().setVisible(true);
        graph.getLegendRenderer().setAlign(LegendRenderer.LegendAlign.TOP);
        graph.getGridLabelRenderer().setLabelVerticalWidth(100);
//        graph.getGridLabelRenderer().setLabelFormatter(new DateAsXAxisLabelFormatter(this));
//        graph.getGridLabelRenderer().setLabelFormatter(new DefaultLabelFormatter() {
//            SimpleDateFormat sdf = new SimpleDateFormat("mm:ss.SSS");
//
//            @Override
//            public String formatLabel(double value, boolean isValueX) {
//                if (isValueX) {
//                    return Double.toString(value);
//                } else {
//                    return super.formatLabel(value, isValueX);
//                }
//            }
//        });
        graph.getGridLabelRenderer().setNumHorizontalLabels(0);
        graph.getGridLabelRenderer().setHumanRounding(true);


        mSeriesAFR = new LineGraphSeries<>();
        mSeriesAFR.setDrawDataPoints(true);
        mSeriesAFR.setDrawBackground(false);
        mSeriesAFR.setTitle("AFR");
        mSeriesAFR.setColor(Color.RED);

        mSeriesAccel = new LineGraphSeries<>();
        mSeriesAccel.setDrawDataPoints(true);
        mSeriesAccel.setDrawBackground(false);
        mSeriesAccel.setTitle("G");
        mSeriesAccel.setColor(Color.GREEN);

        mSeriesSpeed = new LineGraphSeries<>();
        mSeriesSpeed.setDrawDataPoints(true);
        mSeriesSpeed.setDrawAsPath(true);
        mSeriesSpeed.setDrawBackground(false);
        mSeriesSpeed.setTitle("km/h");
        mSeriesSpeed.setColor(Color.LTGRAY);

        graph.addSeries(mSeriesAccel);
        graph.addSeries(mSeriesAFR);
        graph.getSecondScale().addSeries(mSeriesSpeed);
        graph.getSecondScale().setMinY(0);
        graph.getSecondScale().setMaxY(130);
        graph.getGridLabelRenderer().setVerticalLabelsSecondScaleColor(Color.LTGRAY);

        chart = new LineChart(this);
        chart.setMinimumHeight(1500);
        chart.getDescription().setEnabled(false);
        chart.setTouchEnabled(true);
        chart.setDragDecelerationFrictionCoef(0.9f);
        chart.setDragEnabled(true);
        chart.setScaleEnabled(true);
        chart.setAutoScaleMinMaxEnabled(true);
        chart.setDrawGridBackground(false);
        chart.setHighlightPerDragEnabled(false);
//        chart.setBackgroundColor(Color.BLACK);
//        chart.setViewPortOffsets(0f, 0f, 0f, 0f);
//        chart.setMaxVisibleValueCount(10);
//        chart.setVisibleXRangeMaximum(50);

        XAxis xAxis = chart.getXAxis();
        xAxis.setPosition(XAxis.XAxisPosition.TOP_INSIDE);
//        xAxis.setTypeface(tfLight);
        xAxis.setTextSize(8f);
        xAxis.setDrawAxisLine(true);
        xAxis.setDrawGridLines(true);
        xAxis.setTextColor(Color.DKGRAY);
//        xAxis.setCenterAxisLabels(true);
//        xAxis.setGranularity(.1f);
//        xAxis.setGranularityEnabled(false);
        xAxis.setValueFormatter(new ValueFormatter() {
            private final SimpleDateFormat mFormat = new SimpleDateFormat("YYYY-MM-dd HH:mm:ss", Locale.ENGLISH);

            @Override
            public String getFormattedValue(float value) {
                return mFormat.format(new Date((long) value));
            }
        });

        YAxis leftAxis = chart.getAxisLeft();
//        leftAxis.setTypeface(tfLight);
        leftAxis.setTextColor(Color.GREEN);
//        leftAxis.setAxisMaximum(100f);
//        leftAxis.setAxisMinimum(0f);
        leftAxis.setDrawGridLines(true);
        leftAxis.setGridColor(Color.GREEN);
        leftAxis.setDrawLabels(true);
        leftAxis.setEnabled(true);
        leftAxis.setPosition(YAxis.YAxisLabelPosition.INSIDE_CHART);

        YAxis rightAxis = chart.getAxisRight();
//        rightAxis.setTypeface(tfLight);
        rightAxis.setTextColor(Color.BLACK);
//        rightAxis.setAxisMaximum(130f);
//        rightAxis.setAxisMinimum(0f);
        rightAxis.setDrawGridLines(false);
        rightAxis.setGridColor(Color.BLACK);
        rightAxis.setDrawLabels(true);
        rightAxis.setEnabled(true);
        rightAxis.setPosition(YAxis.YAxisLabelPosition.INSIDE_CHART);

        for (int i=0; i<1; i++) {
            entriesAFR.add(new Entry(System.currentTimeMillis(), i));
            entriesAccel.add(new Entry(System.currentTimeMillis(), i));
//            entriesSpeed.add(new Entry(TimeUnit.MILLISECONDS.toHours(System.currentTimeMillis()), i));
            entriesSpeed.add(new Entry(System.currentTimeMillis(), i));
        }
        xAxis.setAxisMinimum(xAxis.getAxisMinimum());

        LineDataSet dataSetAFR = new LineDataSet(entriesAFR, "AFR");
        LineDataSet dataSetAccel = new LineDataSet(entriesAccel, "Accel");
        LineDataSet dataSetSpeed = new LineDataSet(entriesSpeed, "Speed");

        dataSetAFR.setAxisDependency(YAxis.AxisDependency.LEFT);
        dataSetAFR.setColor(Color.GREEN);
        dataSetAFR.setDrawCircles(false);
        dataSetAFR.setLineWidth(2f);
        dataSetAccel.setAxisDependency(YAxis.AxisDependency.LEFT);
        dataSetAccel.setColor(Color.RED);
        dataSetAccel.setDrawCircles(false);
        dataSetAccel.setLineWidth(2f);
        dataSetSpeed.setAxisDependency(YAxis.AxisDependency.RIGHT);
        dataSetSpeed.setColor(Color.BLACK);
        dataSetSpeed.setDrawCircles(false);
        dataSetSpeed.setLineWidth(2f);

//        dataSetSpeed.setAxisDependency(leftAxis.getAxisDependency());

        LineData lineData = new LineData();
        lineData.addDataSet(dataSetAFR);
        lineData.addDataSet(dataSetAccel);
        lineData.addDataSet(dataSetSpeed);

        chart.setData(lineData);
        chart.invalidate();

        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        if (sensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION) != null) {
            accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
            sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_NORMAL);
        }

        lm = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED && ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[] {Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION}, 1);
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        lm.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, this);

        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        registerReceiver(receiver, filter);
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null)
            Log.v(TAG, "Device doesn't support Bluetooth");

            // get a layout defined in xml
        LinearLayout layout = (LinearLayout) findViewById(R.id.layout);
//        layout.addView(chart, LinearLayout.LayoutParams.MATCH_PARENT);
        layout.addView(graph, LinearLayout.LayoutParams.MATCH_PARENT);

    }

    private void updateChart() {
//        chart.moveViewToX(chart.getXChartMax());
        chart.moveViewToX(chart.getXChartMax());
//        chart.fitScreen();
        chart.setVisibleXRangeMaximum(10);
        chart.invalidate();

//        graph.invalidate();
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (event.sensor.getType() == Sensor.TYPE_LINEAR_ACCELERATION) {
//            entriesAccel.add(new Entry(TimeUnit.MILLISECONDS.toHours(event.timestamp), event.values[0]));
//            float ts = System.currentTimeMillis();
            int ts = mCount++;
//            long ts = event.timestamp
            entriesAccel.add(new Entry(ts, event.values[1]));

            double val = abs(event.values[1]) > 9.8/10 ? event.values[1]/9.8 : 0;
//            double val = event.values[1];
            mSeriesAccel.appendData(new DataPoint(ts, val),true, 1000);

            updateChart();
            Log.v(TAG, "Accel: x:" + event.values[0] + " y:" + event.values[1]+ " z:" + event.values[2]);
        }

        if (isBletoothConnected && mSocket.isConnected() && mSocket.getRemoteDevice().getBondState() == BluetoothDevice.BOND_BONDED) {
            try {
//                    Log.v(TAG, "Bluetooth sending CMD=RPM");
                btOutput.write("CMD=RPM;");
//                    Log.v(TAG, "Bluetooth sending CMD=AFR");
                btOutput.write("CMD=AFR;");
                btOutput.flush();
                while (btInput.ready()) {
//                        Log.v(TAG, "Bluetooth starting read line");
                    String line = btInput.readLine();
//                        Log.v(TAG, "Bluetooth read line: " + line);
                    String res = line.split(";")[0];
//                        Log.v(TAG, "Bluetooth read res: " + res);
                    String resArray[] = res.split("=");
//                    Log.v(TAG, "Bluetooth read " + resArray[0] + "=" + resArray[1]);
                    if (resArray[0].contentEquals("AFR")) {
                        mSeriesAFR.appendData(new DataPoint(mCount++, Double.parseDouble(resArray[1])), true, 1000);
                        Log.v(TAG, "AFR: " + resArray[1]);
                    } else if (resArray[0].contentEquals("RPM")) {
//                            mSeriesRPM.appendData(new DataPoint(mCount++, Double.parseDouble(resArray[1])), true, 1000);
                        Log.v(TAG, "RPM: " + resArray[1]);
                    }
                }
            } catch (IOException e) {
                isBletoothConnected = false;
                e.printStackTrace();
            }
        } else {
            isBletoothConnected = false;
            if (bluetoothAdapter != null && !bluetoothAdapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                // https://stackoverflow.com/questions/62671106/onactivityresult-method-is-deprecated-what-is-the-alternative
                startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            }
            if (!bluetoothAdapter.isDiscovering()) {
                Log.v(TAG, "Bluetooth starting discovery");
                bluetoothAdapter.startDiscovery();
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {

    }

    @Override
    public void onLocationChanged(@NonNull Location location) {
        if (location.hasSpeed()){
//            float ts = System.currentTimeMillis();
            int ts = mCount++;
//            long ts = location.getTime();
//            entriesSpeed.add(new Entry(TimeUnit.MILLISECONDS.toHours(ts), location.getSpeed()*3600/1000));
            entriesSpeed.add(new Entry(ts, location.getSpeed()*3600/1000));

            mSeriesSpeed.appendData(new DataPoint(ts, location.getSpeed()*3600/1000), true, 1000);

            updateChart();
            Log.v(TAG, "Speed: " + location.getSpeed()*3600/1000);
        }
    }
}