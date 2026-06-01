package com.nesem.app;

import org.json.JSONException;
import org.json.JSONObject;

public class LocalRomInfo {
    private String uriString;
    private String name;
    private String thumbnailPath;
    private long lastOpened;
    private boolean autoSnapshotTaken;

    public LocalRomInfo(String uriString, String name, String thumbnailPath, long lastOpened, boolean autoSnapshotTaken) {
        this.uriString = uriString;
        this.name = name;
        this.thumbnailPath = thumbnailPath;
        this.lastOpened = lastOpened;
        this.autoSnapshotTaken = autoSnapshotTaken;
    }

    public String getUriString() { return uriString; }
    public String getName() { return name; }
    public String getThumbnailPath() { return thumbnailPath; }
    public long getLastOpened() { return lastOpened; }
    public boolean isAutoSnapshotTaken() { return autoSnapshotTaken; }

    public void setLastOpened(long lastOpened) { this.lastOpened = lastOpened; }
    public void setThumbnailPath(String path) { this.thumbnailPath = path; }
    public void setAutoSnapshotTaken(boolean taken) { this.autoSnapshotTaken = taken; }

    public JSONObject toJsonObject() throws JSONException {
        JSONObject obj = new JSONObject();
        obj.put("uri", uriString);
        obj.put("name", name);
        obj.put("thumbnail", thumbnailPath);
        obj.put("lastOpened", lastOpened);
        obj.put("autoSnapshotTaken", autoSnapshotTaken);
        return obj;
    }

    public static LocalRomInfo fromJsonObject(JSONObject obj) throws JSONException {
        return new LocalRomInfo(
            obj.getString("uri"),
            obj.getString("name"),
            obj.getString("thumbnail"),
            obj.optLong("lastOpened", 0),
            obj.optBoolean("autoSnapshotTaken", false)
        );
    }
}
