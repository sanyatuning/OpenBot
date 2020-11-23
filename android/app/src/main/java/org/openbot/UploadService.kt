package org.openbot

import android.content.Context
import android.util.Log
import com.android.volley.RequestQueue
import com.android.volley.toolbox.Volley
import com.loopj.android.http.AsyncHttpClient
import com.loopj.android.http.AsyncHttpResponseHandler
import com.loopj.android.http.RequestParams
import cz.msebera.android.httpclient.Header
import java.io.File


class UploadService {
  private var queue: RequestQueue? = null

  fun init(context: Context) {
    queue = Volley.newRequestQueue(context)
  }

  fun upload(url: String, file: File) {
    Log.d("Upload", "Start: $url")

    val params = RequestParams()
    params.put("file", file);

    val client = AsyncHttpClient()
    client.post("$url/upload", params, object : AsyncHttpResponseHandler() {
      override fun onStart() {
        // called before request is started
      }

      override fun onSuccess(statusCode: Int, headers: Array<Header?>?, response: ByteArray?) {
        // called when response HTTP status is "200 OK"
      }

      override fun onFailure(statusCode: Int, headers: Array<Header?>?, errorResponse: ByteArray?, e: Throwable?) {
        // called when response HTTP status is "4XX" (eg. 401, 403, 404)
      }

      override fun onRetry(retryNo: Int) {
        // called when request is retried
      }
    })
  }
}