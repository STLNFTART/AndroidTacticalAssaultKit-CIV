package com.primallogic.rmh;

import android.content.Context;
import android.view.View;
import com.atakmap.android.dropdown.DropDown;
import com.atakmap.android.dropdown.DropDownReceiver;
import com.atakmap.android.maps.MapView;
import com.atakmap.coremap.log.Log;

/**
 * RMH Control Drop-Down UI
 *
 * Provides operator interface for:
 * - Tow Body control mode selection
 * - Mission planning and execution
 * - Contact list and details
 * - Statistics display
 *
 * @author Primal Logic
 * @classification UNCLASSIFIED
 */
public class RMHControlDropDown extends DropDownReceiver implements DropDown.OnStateListener {

    private static final String TAG = "RMHControlDropDown";

    private MapView mapView;
    private Context pluginContext;
    private RMHDataManager dataManager;
    private RMHCommsHandler commsHandler;

    public RMHControlDropDown(MapView mapView, Context pluginContext,
                             RMHDataManager dataManager, RMHCommsHandler commsHandler) {
        super(mapView);
        this.mapView = mapView;
        this.pluginContext = pluginContext;
        this.dataManager = dataManager;
        this.commsHandler = commsHandler;

        Log.d(TAG, "RMH Control Drop-Down initialized");
    }

    @Override
    public void onReceive(Context context, android.content.Intent intent) {
        Log.d(TAG, "Showing RMH Control Panel");
        // TODO: Show control UI
    }

    @Override
    protected void disposeImpl() {
        Log.d(TAG, "RMH Control Drop-Down disposed");
    }

    @Override
    public void onDropDownSelectionRemoved() {
    }

    @Override
    public void onDropDownVisible(boolean visible) {
    }

    @Override
    public void onDropDownSizeChanged(double width, double height) {
    }

    @Override
    public void onDropDownClose() {
    }
}
