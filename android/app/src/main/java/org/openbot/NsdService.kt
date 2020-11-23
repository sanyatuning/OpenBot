package org.openbot

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdManager.DiscoveryListener
import android.net.nsd.NsdManager.ResolveListener
import android.net.nsd.NsdServiceInfo
import android.util.Log


class NsdService {

  private lateinit var uploadService: UploadService
  private val SERVICE_TYPE = "_http._tcp."

  private val mDiscoveryListener = object : DiscoveryListener {
    //  Called as soon as service discovery begins.
    override fun onDiscoveryStarted(regType: String) {}
    override fun onServiceFound(service: NsdServiceInfo) {
      // A service was found!  Do something with it.
      val name = service.serviceName
      val type = service.serviceType
      Log.d("NSD", "Service Name=$name")
      Log.d("NSD", "Service Type=$type")
      if (type == SERVICE_TYPE && name.contains("Openbot")) {
        Log.d("NSD", "Service Found @ '$name'")
        mNsdManager!!.resolveService(service, mResolveListener)
      }
    }

    override fun onServiceLost(service: NsdServiceInfo) {
      // When the network service is no longer available.
      // Internal bookkeeping code goes here.
    }

    override fun onDiscoveryStopped(serviceType: String) {}
    override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) {
      mNsdManager!!.stopServiceDiscovery(this)
    }

    override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) {
      mNsdManager!!.stopServiceDiscovery(this)
    }
  }
  private val mResolveListener = object : ResolveListener {
    override fun onResolveFailed(serviceInfo: NsdServiceInfo, errorCode: Int) {
      // Called when the resolve fails.  Use the error code to debug.
      Log.e("NSD", "Resolve failed$errorCode")
    }

    override fun onServiceResolved(serviceInfo: NsdServiceInfo) {
      serverUrl = "http://${serviceInfo.host.hostAddress}:${serviceInfo.port}"
      Log.d("NSD", "Resolved address = $serverUrl")

//      uploadService.upload(serverUrl)
    }
  }

  private var mNsdManager: NsdManager? = null

  var serverUrl = ""

  fun start(context: Context, uploadService: UploadService) {
    Log.d("NSD", "Start discovery")
    mNsdManager = context.getSystemService(Context.NSD_SERVICE) as NsdManager
    mNsdManager!!.discoverServices(SERVICE_TYPE, NsdManager.PROTOCOL_DNS_SD, mDiscoveryListener)
    this.uploadService = uploadService
  }

  fun stop() {
    Log.d("NSD", "Stop discovery")
    mNsdManager!!.stopServiceDiscovery(mDiscoveryListener)
  }
}
