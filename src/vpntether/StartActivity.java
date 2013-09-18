/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance  with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
package vpntether;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.VpnService;
import android.os.Bundle;
import android.util.Log;

import com.google.android.vpntether.R;

/**
 * Launcher for TetherService. Needed to acquire the BIND_VPN_SERVICE
 * permission.
 *
 * @author szym@google.com (Szymon Jakubczak)
 */
public class StartActivity extends Activity {
  private String mServerSockName;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    mServerSockName = getIntent().getStringExtra("SOCKNAME");
    Intent intent = VpnService.prepare(this);
    if (intent != null) {
      startActivityForResult(intent, 0);
    } else {
      onActivityResult(0, RESULT_OK, null);
    }
  }

  @Override
  protected void onActivityResult(int request, int result, Intent data) {
    int message = R.string.result_fail_prepare;
    if (result == RESULT_OK) {
      Intent intent = new Intent(this, TetherService.class)
          .putExtra("SOCKNAME", mServerSockName);
      message = (startService(intent) != null) ?
                    R.string.result_success : R.string.result_fail_start;
    }
    new AlertDialog.Builder(this)
      .setTitle(R.string.app_name)
      .setMessage(message)
      .setOnCancelListener(new DialogInterface.OnCancelListener() {
        @Override
        public void onCancel(DialogInterface dialog) {
          finish();
        }
      })
      .create()
      .show();
  }
}
