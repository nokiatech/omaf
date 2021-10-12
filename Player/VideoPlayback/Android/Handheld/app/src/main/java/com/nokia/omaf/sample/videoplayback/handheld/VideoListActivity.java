
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
package com.nokia.omaf.sample.videoplayback.handheld;

import android.content.Intent;
import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

public class VideoListActivity extends Activity {

    private Map<String, String> mVideos;
    // Array of strings...
    String[] mobileArray;
    String[] fileSuffix = {".heic", ".mp4"};
    String uriSuffix = ".uri";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mVideos = ParseAssets();
        mobileArray = mVideos.keySet().toArray(new String[mVideos.keySet().size()]);
        System.out.println("Assets size:" + mVideos.size());

        setContentView(R.layout.activity_video_list);

        ArrayAdapter adapter = new ArrayAdapter<String>(this,
                R.layout.activity_listview, mobileArray);

        ListView listView = (ListView) findViewById(R.id.mobile_list);
        listView.setAdapter(adapter);
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, final View view,
                                    int position, long id) {
                final String item = (String) parent.getItemAtPosition(position);
                if(mVideos.containsKey(item)) {
                    Intent intent = new Intent(VideoListActivity.this, MainActivity.class);
                    intent.putExtra("uri",mVideos.get(item));
                    startActivity(intent);
                }
                else
                {
                    System.out.println("Could not find item: " + item);
                }
            }

        });
    }

    // Internal functions
    Map<String, String> ParseAssets()
    {
        Map<String,String> map =  new HashMap<String,String>();
        String [] list;
        try {
            list = getResources().getAssets().list("");
            if (list.length > 0) {
                // This is a folder
                for (String file : list) {
                    File f = new File(file);
                    Boolean added = false;
                    for(String type : fileSuffix) {
                        if (f.getName().endsWith(type)) {
                            map.put(f.getName(), "asset://" + file);
                            added = true;
                            break;
                        }
                    }
                    if (!added && f.getName().endsWith(uriSuffix)) {
                        String videoUri;

                        BufferedReader reader = null;
                        try {
                            // prepare the file for reading
                            reader = new BufferedReader(new InputStreamReader(getAssets().open(file)));

                            // read every line of the file into the line-variable, on line at the time
                            do {
                                videoUri = reader.readLine();

                                if (videoUri == null || videoUri.length() == 0) { continue; }

                                if (videoUri.contains("://")) {
                                    map.put(file, videoUri);
                                }
                            } while (videoUri != null);
                        } catch (IOException e) {
                            continue;
                        }
                        finally {
                            if (reader != null) {
                                try {
                                    reader.close();
                                } catch (IOException e) {
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
        } catch (IOException e) {
            return map;
        }

        return map;
    }
}
