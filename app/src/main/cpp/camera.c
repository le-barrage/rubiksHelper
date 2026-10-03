#include "camera.h"

#include "deps/raymob/raymob.h"
#include "raylib.h"

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PREVIEW_WIDTH  1280
#define MAX_PREVIEW_HEIGHT 960

static ACameraManager *manager                 = NULL;
static ACameraDevice *device                   = NULL;
static AImageReader *reader                    = NULL;
static ACaptureSessionOutputContainer *outputs = NULL;
static ACaptureSessionOutput *output           = NULL;
static ACameraOutputTarget *target             = NULL;
static ACaptureRequest *request                = NULL;
static ACameraCaptureSession *session          = NULL;

static bool pendingPermission = false;
static bool opened            = false;

static int sensorOrientation = 0;
static int srcWidth = 0, srcHeight = 0;

static Texture2D texture     = { 0 };
static unsigned char *pixels = NULL;
static bool hasFrame         = false;

/* Permission */

static bool hasCameraPermission (void)
{
    JNIEnv *env           = AttachCurrentThread();
    jobject nativLoadInst = GetNativeLoaderInstance();
    jclass cls            = (*env)->GetObjectClass(env, nativLoadInst);
    jmethodID check       = (*env)->GetMethodID(env, cls, "checkSelfPermission", "(Ljava/lang/String;)I");
    jstring perm          = (*env)->NewStringUTF(env, "android.permission.CAMERA");
    jint result           = (*env)->CallIntMethod(env, nativLoadInst, check, perm);
    (*env)->DeleteLocalRef(env, perm);
    (*env)->DeleteLocalRef(env, cls);
    DetachCurrentThread();
    return result == 0;
}

static void requestCameraPermission (void)
{
    JNIEnv *env           = AttachCurrentThread();
    jobject nativLoadInst = GetNativeLoaderInstance();
    jclass cls            = (*env)->GetObjectClass(env, nativLoadInst);
    jmethodID request     = (*env)->GetMethodID(env, cls, "requestPermissions", "([Ljava/lang/String;I)V");
    jclass strCls         = (*env)->FindClass(env, "java/lang/String");
    jstring perm          = (*env)->NewStringUTF(env, "android.permission.CAMERA");
    jobjectArray arr      = (*env)->NewObjectArray(env, 1, strCls, perm);
    (*env)->CallVoidMethod(env, nativLoadInst, request, arr, 1);
    (*env)->DeleteLocalRef(env, arr);
    (*env)->DeleteLocalRef(env, perm);
    (*env)->DeleteLocalRef(env, strCls);
    (*env)->DeleteLocalRef(env, cls);
    DetachCurrentThread();
}

/* Camera callbacks */

static void onDisconnected (void *ctx, ACameraDevice *dev) { TraceLog(LOG_WARNING, "CAMERA: disconnected"); }

static void onError (void *ctx, ACameraDevice *dev, int err) { TraceLog(LOG_ERROR, "CAMERA: device error %d", err); }

static void onSessionClosed (void *ctx, ACameraCaptureSession *s) { TraceLog(LOG_INFO, "CAMERA: session closed"); }
static void onSessionReady (void *ctx, ACameraCaptureSession *s) { TraceLog(LOG_INFO, "CAMERA: session ready"); }
static void onSessionActive (void *ctx, ACameraCaptureSession *s) { TraceLog(LOG_INFO, "CAMERA: session active"); }

static ACameraDevice_StateCallbacks deviceCallbacks = {
    .context        = NULL,
    .onDisconnected = onDisconnected,
    .onError        = onError,
};

static ACameraCaptureSession_stateCallbacks sessionCallbacks = {
    .context  = NULL,
    .onClosed = onSessionClosed,
    .onReady  = onSessionReady,
    .onActive = onSessionActive,
};

/* Setup */

// Finds the back camera, its orientation and the biggest YUV size under the max preview size
static bool findBackCamera (char *outId, size_t idSize)
{
    ACameraIdList *ids = NULL;
    if (ACameraManager_getCameraIdList(manager, &ids) != ACAMERA_OK) return false;

    bool found = false;
    for (int i = 0; i < ids->numCameras && !found; i++) {
        ACameraMetadata *meta = NULL;
        if (ACameraManager_getCameraCharacteristics(manager, ids->cameraIds[i], &meta) != ACAMERA_OK) continue;

        ACameraMetadata_const_entry entry = { 0 };
        ACameraMetadata_getConstEntry(meta, ACAMERA_LENS_FACING, &entry);
        if (entry.count == 0 || entry.data.u8[0] != ACAMERA_LENS_FACING_BACK) {
            ACameraMetadata_free(meta);
            continue;
        }

        ACameraMetadata_getConstEntry(meta, ACAMERA_SENSOR_ORIENTATION, &entry);
        sensorOrientation = entry.count > 0 ? entry.data.i32[0] : 0;

        // Entries are (format, width, height, isInput)
        srcWidth = srcHeight = 0;
        ACameraMetadata_getConstEntry(meta, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);
        for (uint32_t j = 0; j + 3 < entry.count; j += 4) {
            int32_t format = entry.data.i32[j], w = entry.data.i32[j + 1], h = entry.data.i32[j + 2];
            if (entry.data.i32[j + 3] || format != AIMAGE_FORMAT_YUV_420_888) continue;
            if (w > MAX_PREVIEW_WIDTH || h > MAX_PREVIEW_HEIGHT) continue;
            if (w * h > srcWidth * srcHeight) {
                srcWidth  = w;
                srcHeight = h;
            }
        }

        if (srcWidth > 0) {
            strncpy(outId, ids->cameraIds[i], idSize - 1);
            outId[idSize - 1] = '\0';
            found             = true;
        }
        ACameraMetadata_free(meta);
    }

    ACameraManager_deleteCameraIdList(ids);
    return found;
}

static bool startCamera (void)
{
    manager = ACameraManager_create();

    char id[64];
    if (!findBackCamera(id, sizeof(id))) {
        TraceLog(LOG_ERROR, "CAMERA: no back camera found");
        return false;
    }
    TraceLog(LOG_INFO, "CAMERA: id %s, %dx%d, orientation %d", id, srcWidth, srcHeight, sensorOrientation);

    if (ACameraManager_openCamera(manager, id, &deviceCallbacks, &device) != ACAMERA_OK) return false;
    if (AImageReader_new(srcWidth, srcHeight, AIMAGE_FORMAT_YUV_420_888, 2, &reader) != AMEDIA_OK) return false;

    ANativeWindow *window = NULL;
    AImageReader_getWindow(reader, &window);

    ACaptureSessionOutputContainer_create(&outputs);
    ACaptureSessionOutput_create(window, &output);
    ACaptureSessionOutputContainer_add(outputs, output);

    if (ACameraDevice_createCaptureRequest(device, TEMPLATE_PREVIEW, &request) != ACAMERA_OK) return false;
    ACameraOutputTarget_create(window, &target);
    ACaptureRequest_addTarget(request, target);

    if (ACameraDevice_createCaptureSession(device, outputs, &sessionCallbacks, &session) != ACAMERA_OK) return false;
    if (ACameraCaptureSession_setRepeatingRequest(session, NULL, 1, &request, NULL) != ACAMERA_OK) return false;

    // Rotated to match the portrait screen
    bool swap = sensorOrientation == 90 || sensorOrientation == 270;
    int dstW  = swap ? srcHeight : srcWidth;
    int dstH  = swap ? srcWidth : srcHeight;

    pixels = malloc(dstW * dstH * 4);
    Image image
        = { .data = pixels, .width = dstW, .height = dstH, .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    texture = LoadTextureFromImage(image);
    return true;
}

/* Frame conversion */

static unsigned char clamp (int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }

static void convertFrame (AImage *image)
{
    uint8_t *yData, *uData, *vData;
    int yLen, uLen, vLen;
    int32_t yStride, uvStride, uvPixelStride;
    AImage_getPlaneData(image, 0, &yData, &yLen);
    AImage_getPlaneData(image, 1, &uData, &uLen);
    AImage_getPlaneData(image, 2, &vData, &vLen);
    AImage_getPlaneRowStride(image, 0, &yStride);
    AImage_getPlaneRowStride(image, 1, &uvStride);
    AImage_getPlanePixelStride(image, 1, &uvPixelStride);

    int dstW = texture.width, dstH = texture.height;
    for (int dy = 0; dy < dstH; dy++) {
        for (int dx = 0; dx < dstW; dx++) {
            // Clockwise rotation by the sensor orientation
            int sx, sy;
            switch (sensorOrientation) {
                case 90:
                    sx = dy;
                    sy = srcHeight - 1 - dx;
                    break;
                case 180:
                    sx = srcWidth - 1 - dx;
                    sy = srcHeight - 1 - dy;
                    break;
                case 270:
                    sx = srcWidth - 1 - dy;
                    sy = dx;
                    break;
                default:
                    sx = dx;
                    sy = dy;
                    break;
            }

            int y        = yData[sy * yStride + sx];
            int uvOffset = (sy / 2) * uvStride + (sx / 2) * uvPixelStride;
            int u        = uData[uvOffset] - 128;
            int v        = vData[uvOffset] - 128;

            unsigned char *p = &pixels[(dy * dstW + dx) * 4];
            p[0]             = clamp(y + ((359 * v) >> 8));
            p[1]             = clamp(y - ((88 * u + 183 * v) >> 8));
            p[2]             = clamp(y + ((454 * u) >> 8));
            p[3]             = 255;
        }
    }
}

/* Public API */

void cameraOpen (void)
{
    if (opened || pendingPermission) return;
    if (!hasCameraPermission()) requestCameraPermission();
    pendingPermission = true;
}

void cameraClose (void)
{
    pendingPermission = false;
    opened            = false;
    hasFrame          = false;

    if (session) {
        ACameraCaptureSession_stopRepeating(session);
        ACameraCaptureSession_close(session);
        session = NULL;
    }
    if (request) {
        ACaptureRequest_free(request);
        request = NULL;
    }
    if (target) {
        ACameraOutputTarget_free(target);
        target = NULL;
    }
    if (outputs) {
        if (output) ACaptureSessionOutputContainer_remove(outputs, output);
        ACaptureSessionOutputContainer_free(outputs);
        outputs = NULL;
    }
    if (output) {
        ACaptureSessionOutput_free(output);
        output = NULL;
    }
    if (device) {
        ACameraDevice_close(device);
        device = NULL;
    }
    if (reader) {
        AImageReader_delete(reader);
        reader = NULL;
    }
    if (manager) {
        ACameraManager_delete(manager);
        manager = NULL;
    }
    if (texture.id) {
        UnloadTexture(texture);
        texture = (Texture2D){ 0 };
    }
    free(pixels);
    pixels = NULL;
}

bool cameraIsActive (void) { return opened || pendingPermission; }

bool cameraUpdate (void)
{
    if (pendingPermission && hasCameraPermission()) {
        pendingPermission = false;
        opened            = startCamera();
        if (!opened) cameraClose();
    }
    if (!opened) return false;

    AImage *image = NULL;
    if (AImageReader_acquireLatestImage(reader, &image) == AMEDIA_OK && image) {
        convertFrame(image);
        AImage_delete(image);
        UpdateTexture(texture, pixels);
        hasFrame = true;
    }
    return hasFrame;
}

Texture2D cameraGetTexture (void) { return texture; }

Color cameraSampleColor (int cx, int cy, int radius)
{
    if (!hasFrame) return BLANK;

    int r = 0, g = 0, b = 0, n = 0;
    for (int y = cy - radius; y <= cy + radius; y++) {
        if (y < 0 || y >= texture.height) continue;
        for (int x = cx - radius; x <= cx + radius; x++) {
            if (x < 0 || x >= texture.width) continue;
            unsigned char *p = &pixels[(y * texture.width + x) * 4];
            r += p[0];
            g += p[1];
            b += p[2];
            n++;
        }
    }
    if (n == 0) return BLANK;
    return (Color){ r / n, g / n, b / n, 255 };
}
