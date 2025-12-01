package com.primallogic.rmh;

import android.content.Context;
import android.content.Intent;
import android.view.View;

import com.atakmap.android.dropdown.DropDownMapComponent;
import com.atakmap.android.dropdown.DropDownReceiver;
import com.atakmap.android.ipc.AtakBroadcast;
import com.atakmap.android.maps.MapView;
import com.atakmap.android.widgets.LinearLayoutWidget;
import com.atakmap.android.widgets.MapWidget;
import com.atakmap.android.widgets.RootLayoutWidget;
import com.atakmap.coremap.log.Log;

/**
 * RMH Map Component
 *
 * Core component managing RMH mission visualization, C2 interface,
 * and integration with ATAK map view.
 *
 * Features:
 * - Tow Body position and track visualization
 * - Mine contact display with classification confidence
 * - Search area and coverage overlay
 * - Mission control interface
 *
 * @author Primal Logic
 * @classification UNCLASSIFIED
 */
public class RMHMapComponent extends DropDownMapComponent {

    private static final String TAG = "RMHMapComponent";
    private static final String SHOW_RMH_CONTROL = "com.primallogic.rmh.SHOW_CONTROL";

    private Context pluginContext;
    private MapView mapView;

    private RMHControlDropDown controlDropDown;
    private RMHMapOverlay mapOverlay;
    private RMHDataManager dataManager;
    private RMHCommsHandler commsHandler;

    public void onCreate(Context pluginContext, Intent intent, MapView mapView) {
        this.pluginContext = pluginContext;
        this.mapView = mapView;

        Log.d(TAG, "Initializing RMH Map Component");

        // Initialize data manager (handles TB state, contacts, mission data)
        dataManager = new RMHDataManager();

        // Initialize communications handler
        commsHandler = new RMHCommsHandler(dataManager);
        commsHandler.start();

        // Initialize map overlay for visualization
        mapOverlay = new RMHMapOverlay(mapView, dataManager);
        mapView.getMapOverlayManager().addOverlay(mapOverlay);

        // Initialize control drop-down UI
        controlDropDown = new RMHControlDropDown(mapView, pluginContext, dataManager, commsHandler);

        // Register to show control panel
        AtakBroadcast.getInstance().registerReceiver(
            controlDropDown,
            new AtakBroadcast.DocumentedIntentFilter(SHOW_RMH_CONTROL)
        );

        // Add RMH tool button to ATAK toolbar
        addRMHToolbarButton();

        Log.i(TAG, "RMH Map Component initialized");
    }

    public void onDestroy(Context pluginContext, MapView mapView) {
        Log.d(TAG, "Destroying RMH Map Component");

        if (commsHandler != null) {
            commsHandler.stop();
            commsHandler = null;
        }

        if (controlDropDown != null) {
            AtakBroadcast.getInstance().unregisterReceiver(controlDropDown);
            controlDropDown.dispose();
            controlDropDown = null;
        }

        if (mapOverlay != null) {
            mapView.getMapOverlayManager().removeOverlay(mapOverlay);
            mapOverlay = null;
        }

        dataManager = null;

        Log.i(TAG, "RMH Map Component destroyed");
    }

    public void onStart(Context pluginContext, MapView mapView) {
        Log.d(TAG, "RMH Map Component: onStart");
    }

    public void onStop(Context pluginContext, MapView mapView) {
        Log.d(TAG, "RMH Map Component: onStop");
    }

    public void onPause(Context pluginContext, MapView mapView) {
        Log.d(TAG, "RMH Map Component: onPause");
        if (commsHandler != null) {
            commsHandler.pause();
        }
    }

    public void onResume(Context pluginContext, MapView mapView) {
        Log.d(TAG, "RMH Map Component: onResume");
        if (commsHandler != null) {
            commsHandler.resume();
        }
    }

    /**
     * Add RMH control button to ATAK toolbar
     */
    private void addRMHToolbarButton() {
        Log.d(TAG, "Adding RMH toolbar button");

        // Get root layout widget
        RootLayoutWidget root = (RootLayoutWidget) mapView.getComponentExtra("rootLayoutWidget");
        if (root == null) {
            Log.e(TAG, "Cannot find root layout widget");
            return;
        }

        // Create RMH button widget
        // This would normally create a custom MapWidget button
        // For now, we'll trigger via intent
        Intent showIntent = new Intent(SHOW_RMH_CONTROL);

        Log.i(TAG, "RMH toolbar button added - use intent: " + SHOW_RMH_CONTROL);
    }
}
