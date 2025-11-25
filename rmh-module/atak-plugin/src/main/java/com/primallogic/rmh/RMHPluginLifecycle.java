package com.primallogic.rmh;

import android.content.Context;
import android.content.Intent;
import com.atakmap.android.ipc.AtakBroadcast;
import com.atakmap.android.maps.MapView;
import com.atakmap.coremap.log.Log;

import transapps.maps.plugin.lifecycle.Lifecycle;

/**
 * RMH Plugin Lifecycle Handler
 *
 * Manages the lifecycle of the Remote Minehunting (RMH) ATAK plugin.
 * Handles plugin initialization, UI registration, and cleanup.
 *
 * @author Primal Logic
 * @classification UNCLASSIFIED
 */
public class RMHPluginLifecycle implements Lifecycle {

    private static final String TAG = "RMHPluginLifecycle";

    private Context pluginContext;
    private MapView mapView;
    private RMHMapComponent rmhMapComponent;

    @Override
    public void onCreate(Context context, Intent intent, MapView mapView) {
        this.pluginContext = context;
        this.mapView = mapView;

        Log.d(TAG, "RMH Plugin: onCreate()");

        // Initialize RMH map component
        rmhMapComponent = new RMHMapComponent();
        rmhMapComponent.onCreate(pluginContext, intent, mapView);

        Log.i(TAG, "RMH Plugin initialized successfully");
    }

    @Override
    public void onDestroy(Context context, MapView mapView) {
        Log.d(TAG, "RMH Plugin: onDestroy()");

        if (rmhMapComponent != null) {
            rmhMapComponent.onDestroy(pluginContext, mapView);
            rmhMapComponent = null;
        }

        this.pluginContext = null;
        this.mapView = null;

        Log.i(TAG, "RMH Plugin destroyed");
    }

    @Override
    public void onStart(Context context, MapView mapView) {
        Log.d(TAG, "RMH Plugin: onStart()");

        if (rmhMapComponent != null) {
            rmhMapComponent.onStart(pluginContext, mapView);
        }
    }

    @Override
    public void onStop(Context context, MapView mapView) {
        Log.d(TAG, "RMH Plugin: onStop()");

        if (rmhMapComponent != null) {
            rmhMapComponent.onStop(pluginContext, mapView);
        }
    }

    @Override
    public void onPause(Context context, MapView mapView) {
        Log.d(TAG, "RMH Plugin: onPause()");

        if (rmhMapComponent != null) {
            rmhMapComponent.onPause(pluginContext, mapView);
        }
    }

    @Override
    public void onResume(Context context, MapView mapView) {
        Log.d(TAG, "RMH Plugin: onResume()");

        if (rmhMapComponent != null) {
            rmhMapComponent.onResume(pluginContext, mapView);
        }
    }
}
