package com.example.twsnoisecanceller

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.media.AudioManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import android.widget.ToggleButton
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class MainActivity : AppCompatActivity() {

    private val TAG = "TWSNoiseCanceller"
    private val PERMISSION_REQUEST_CODE = 1001

    private lateinit var toggleButton: ToggleButton
    private lateinit var audioManager: AudioManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        toggleButton = findViewById(R.id.toggleButton)
        audioManager = getSystemService(Context.AUDIO_SERVICE) as AudioManager

        toggleButton.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                if (checkPermissions()) {
                    startNoiseCancellation()
                } else {
                    toggleButton.isChecked = false
                    requestPermissions()
                }
            } else {
                stopNoiseCancellation()
            }
        }
    }

    private fun startNoiseCancellation() {
        Log.d(TAG, "Starting Noise Cancellation")
        
        // Route audio through Bluetooth SCO (True Wireless Stereo mic)
        try {
            audioManager.mode = AudioManager.MODE_IN_COMMUNICATION
            audioManager.startBluetoothSco()
            audioManager.isBluetoothScoOn = true
            
            // Allow time for SCO connection to establish before starting DSP
            // In a real app, listen for ACTION_SCO_AUDIO_STATE_UPDATED broadcast
            Thread.sleep(1000) 
            
            if (startDenoise()) {
                Toast.makeText(this, "ANC Started", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(this, "Failed to start native audio", Toast.LENGTH_SHORT).show()
                stopNoiseCancellation()
                toggleButton.isChecked = false
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error starting ANC: ${e.message}")
            stopNoiseCancellation()
            toggleButton.isChecked = false
        }
    }

    private fun stopNoiseCancellation() {
        Log.d(TAG, "Stopping Noise Cancellation")
        
        stopDenoise()
        
        try {
            audioManager.isBluetoothScoOn = false
            audioManager.stopBluetoothSco()
            audioManager.mode = AudioManager.MODE_NORMAL
        } catch (e: Exception) {
            Log.e(TAG, "Error stopping SCO: ${e.message}")
        }
    }

    private fun checkPermissions(): Boolean {
        val permissions = mutableListOf(
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.MODIFY_AUDIO_SETTINGS
        )

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions.add(Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            permissions.add(Manifest.permission.BLUETOOTH)
            permissions.add(Manifest.permission.BLUETOOTH_ADMIN)
        }

        for (p in permissions) {
            if (ContextCompat.checkSelfPermission(this, p) != PackageManager.PERMISSION_GRANTED) {
                return false
            }
        }
        return true
    }

    private fun requestPermissions() {
        val permissions = mutableListOf(
            Manifest.permission.RECORD_AUDIO,
            Manifest.permission.MODIFY_AUDIO_SETTINGS
        )

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions.add(Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            permissions.add(Manifest.permission.BLUETOOTH)
            permissions.add(Manifest.permission.BLUETOOTH_ADMIN)
        }

        ActivityCompat.requestPermissions(
            this,
            permissions.toTypedArray(),
            PERMISSION_REQUEST_CODE
        )
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.isNotEmpty() && grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
                toggleButton.isChecked = true // Triggers the listener
            } else {
                Toast.makeText(this, "Permissions required for ANC", Toast.LENGTH_LONG).show()
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        stopNoiseCancellation()
    }

    /**
     * A native method that is implemented by the 'twsnoisecanceller' native library,
     * which is packaged with this application.
     */
    private external fun startDenoise(): Boolean
    private external fun stopDenoise()

    companion object {
        // Used to load the 'twsnoisecanceller' library on application startup.
        init {
            System.loadLibrary("twsnoisecanceller")
        }
    }
}
