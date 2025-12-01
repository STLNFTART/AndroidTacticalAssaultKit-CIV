package com.primallogic.rmh;

import com.atakmap.coremap.log.Log;
import com.atakmap.coremap.maps.coords.GeoPoint;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * RMH Data Manager
 *
 * Central data store for RMH mission data including:
 * - Tow Body state and telemetry
 * - Detected mine contacts
 * - Search area definition
 * - Mission statistics
 *
 * Thread-safe for concurrent access from comms and UI threads.
 *
 * @author Primal Logic
 * @classification UNCLASSIFIED
 */
public class RMHDataManager {

    private static final String TAG = "RMHDataManager";

    // Tow Body state
    private volatile TowBodyState towBodyState;
    private final List<GeoPoint> towBodyTrack = new ArrayList<>();
    private static final int MAX_TRACK_POINTS = 1000;

    // Mine contacts
    private final Map<String, MineContact> contacts = new ConcurrentHashMap<>();

    // Search area
    private SearchArea searchArea;

    // Mission statistics
    private MissionStats stats = new MissionStats();

    // Data update listeners
    private final List<DataUpdateListener> listeners = new ArrayList<>();

    public RMHDataManager() {
        Log.d(TAG, "RMHDataManager initialized");
    }

    /**
     * Update Tow Body state
     */
    public synchronized void updateTowBodyState(TowBodyState state) {
        this.towBodyState = state;

        // Add to track history
        towBodyTrack.add(state.getPosition());
        if (towBodyTrack.size() > MAX_TRACK_POINTS) {
            towBodyTrack.remove(0);
        }

        // Update statistics
        stats.updatePosition(state.getPosition());

        notifyListeners(UpdateType.TOW_BODY_STATE);
    }

    /**
     * Get current Tow Body state
     */
    public TowBodyState getTowBodyState() {
        return towBodyState;
    }

    /**
     * Get Tow Body track history
     */
    public synchronized List<GeoPoint> getTowBodyTrack() {
        return new ArrayList<>(towBodyTrack);
    }

    /**
     * Add or update mine contact
     */
    public void addContact(MineContact contact) {
        contacts.put(contact.getId(), contact);
        stats.incrementContactCount();

        Log.i(TAG, "Contact added: " + contact.getId() +
                   " Class: " + contact.getClassification() +
                   " Confidence: " + contact.getConfidence());

        notifyListeners(UpdateType.CONTACT_ADDED);
    }

    /**
     * Remove contact
     */
    public void removeContact(String contactId) {
        if (contacts.remove(contactId) != null) {
            Log.i(TAG, "Contact removed: " + contactId);
            notifyListeners(UpdateType.CONTACT_REMOVED);
        }
    }

    /**
     * Get all contacts
     */
    public Map<String, MineContact> getContacts() {
        return new HashMap<>(contacts);
    }

    /**
     * Get contact by ID
     */
    public MineContact getContact(String contactId) {
        return contacts.get(contactId);
    }

    /**
     * Set search area
     */
    public void setSearchArea(SearchArea area) {
        this.searchArea = area;
        Log.i(TAG, "Search area set: " + area.getDescription());
        notifyListeners(UpdateType.SEARCH_AREA_UPDATED);
    }

    /**
     * Get search area
     */
    public SearchArea getSearchArea() {
        return searchArea;
    }

    /**
     * Get mission statistics
     */
    public MissionStats getStatistics() {
        return stats;
    }

    /**
     * Register data update listener
     */
    public void addListener(DataUpdateListener listener) {
        synchronized (listeners) {
            listeners.add(listener);
        }
    }

    /**
     * Unregister data update listener
     */
    public void removeListener(DataUpdateListener listener) {
        synchronized (listeners) {
            listeners.remove(listener);
        }
    }

    /**
     * Notify all listeners of data update
     */
    private void notifyListeners(UpdateType type) {
        synchronized (listeners) {
            for (DataUpdateListener listener : listeners) {
                listener.onDataUpdate(type);
            }
        }
    }

    /**
     * Clear all data (reset mission)
     */
    public synchronized void clear() {
        towBodyState = null;
        towBodyTrack.clear();
        contacts.clear();
        searchArea = null;
        stats = new MissionStats();
        Log.i(TAG, "Data cleared");
        notifyListeners(UpdateType.CLEARED);
    }

    // Inner classes and enums

    public enum UpdateType {
        TOW_BODY_STATE,
        CONTACT_ADDED,
        CONTACT_REMOVED,
        SEARCH_AREA_UPDATED,
        CLEARED
    }

    public interface DataUpdateListener {
        void onDataUpdate(UpdateType type);
    }

    /**
     * Tow Body state data
     */
    public static class TowBodyState {
        private final GeoPoint position;
        private final double depth;
        private final double altitude;
        private final double heading;
        private final double speed;
        private final String controlMode;
        private final String missionPhase;
        private final long timestamp;

        public TowBodyState(GeoPoint position, double depth, double altitude,
                           double heading, double speed, String controlMode,
                           String missionPhase, long timestamp) {
            this.position = position;
            this.depth = depth;
            this.altitude = altitude;
            this.heading = heading;
            this.speed = speed;
            this.controlMode = controlMode;
            this.missionPhase = missionPhase;
            this.timestamp = timestamp;
        }

        public GeoPoint getPosition() { return position; }
        public double getDepth() { return depth; }
        public double getAltitude() { return altitude; }
        public double getHeading() { return heading; }
        public double getSpeed() { return speed; }
        public String getControlMode() { return controlMode; }
        public String getMissionPhase() { return missionPhase; }
        public long getTimestamp() { return timestamp; }
    }

    /**
     * Mine contact data
     */
    public static class MineContact {
        private final String id;
        private final GeoPoint position;
        private final String classification;
        private final float confidence;
        private final long detectionTime;
        private final String description;

        public MineContact(String id, GeoPoint position, String classification,
                          float confidence, long detectionTime, String description) {
            this.id = id;
            this.position = position;
            this.classification = classification;
            this.confidence = confidence;
            this.detectionTime = detectionTime;
            this.description = description;
        }

        public String getId() { return id; }
        public GeoPoint getPosition() { return position; }
        public String getClassification() { return classification; }
        public float getConfidence() { return confidence; }
        public long getDetectionTime() { return detectionTime; }
        public String getDescription() { return description; }
    }

    /**
     * Search area definition
     */
    public static class SearchArea {
        private final List<GeoPoint> boundary;
        private final String pattern;
        private final double trackSpacing;
        private final String description;

        public SearchArea(List<GeoPoint> boundary, String pattern,
                         double trackSpacing, String description) {
            this.boundary = boundary;
            this.pattern = pattern;
            this.trackSpacing = trackSpacing;
            this.description = description;
        }

        public List<GeoPoint> getBoundary() { return boundary; }
        public String getPattern() { return pattern; }
        public double getTrackSpacing() { return trackSpacing; }
        public String getDescription() { return description; }
    }

    /**
     * Mission statistics
     */
    public static class MissionStats {
        private long missionStartTime = 0;
        private int contactCount = 0;
        private double distanceTraveled = 0.0;
        private double areaCovered = 0.0;
        private GeoPoint lastPosition = null;

        public void updatePosition(GeoPoint newPosition) {
            if (missionStartTime == 0) {
                missionStartTime = System.currentTimeMillis();
            }

            if (lastPosition != null) {
                distanceTraveled += lastPosition.distanceTo(newPosition);
            }
            lastPosition = newPosition;
        }

        public void incrementContactCount() {
            contactCount++;
        }

        public long getMissionDuration() {
            if (missionStartTime == 0) return 0;
            return System.currentTimeMillis() - missionStartTime;
        }

        public int getContactCount() { return contactCount; }
        public double getDistanceTraveled() { return distanceTraveled; }
        public double getAreaCovered() { return areaCovered; }
    }
}
