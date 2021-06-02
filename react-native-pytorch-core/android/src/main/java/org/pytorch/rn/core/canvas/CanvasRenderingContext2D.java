/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package org.pytorch.rn.core.canvas;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.text.TextPaint;
import android.util.DisplayMetrics;
import com.facebook.react.bridge.ReadableMap;
import java.util.Stack;

public class CanvasRenderingContext2D {
  private final DrawingCanvasView mDrawingCanvasView;
  private final DisplayMetrics mDisplayMetrics;
  private final float mPixelDensity;
  private final Stack<CanvasState> mSavedStates;
  private Paint mFillPaint;
  private Paint mStrokePaint;
  private Paint mBitmapPaint;
  private Paint mClearPaint;
  private TextPaint mTextFillPaint;
  private Bitmap mBitmap;
  private Canvas mCanvas;
  private Path mPath;

  public CanvasRenderingContext2D(DrawingCanvasView drawingCanvasView) {
    mDrawingCanvasView = drawingCanvasView;
    mSavedStates = new Stack<>();

    mDisplayMetrics = mDrawingCanvasView.getResources().getDisplayMetrics();
    mPixelDensity = mDisplayMetrics.density;

    initPaint();
    init();
  }

  private void initPaint() {
    mFillPaint = new Paint();
    mFillPaint.setAntiAlias(true);
    mFillPaint.setDither(true);
    mFillPaint.setColor(Color.BLACK);
    mFillPaint.setStyle(Paint.Style.FILL);

    mStrokePaint = new Paint();
    mStrokePaint.setAntiAlias(true);
    mStrokePaint.setDither(true);
    mStrokePaint.setColor(Color.BLACK);
    mStrokePaint.setStyle(Paint.Style.STROKE);
    // Initialize stroke width to be pixel density, which matches 1px for a web canvas.
    mStrokePaint.setStrokeWidth(mPixelDensity);

    mClearPaint = new Paint();
    mClearPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));

    mBitmapPaint = new Paint();
    mBitmapPaint.setFilterBitmap(true);

    mTextFillPaint = new TextPaint();
    mTextFillPaint.set(mFillPaint);
    mTextFillPaint.setStyle(Paint.Style.FILL);
    // The default font size for web canvas is 10px.
    mTextFillPaint.setTextSize(dpToPx(10));
    // Initialize stroke width to be pixel density, which matches 1px for a web canvas.
    mTextFillPaint.setStrokeWidth(mPixelDensity);
  }

  private void init() {
    mPath = new Path();
    mBitmap =
        Bitmap.createBitmap(
            mDisplayMetrics.widthPixels, mDisplayMetrics.heightPixels, Bitmap.Config.ARGB_8888);
    mCanvas = new Canvas(mBitmap);
  }

  protected void onDraw(Canvas canvas) {
    canvas.drawBitmap(mBitmap, 0, 0, mBitmapPaint);
  }

  public void setFillStyle(int color) {
    mFillPaint.setColor(color);
    mTextFillPaint.setColor(color);
  }

  public void setStrokeStyle(int color) {
    mStrokePaint.setColor(color);
  }

  public void setLineWidth(float width) {
    final float strokeWidth = dpToPx(width);
    mStrokePaint.setStrokeWidth(strokeWidth);
    mTextFillPaint.setStrokeWidth(strokeWidth);
  }

  public void setLineCap(String lineCap) {
    Paint.Cap cap = null;
    switch (lineCap) {
      case "butt":
        cap = Paint.Cap.BUTT;
        break;
      case "round":
        cap = Paint.Cap.ROUND;
        break;
      case "square":
        cap = Paint.Cap.SQUARE;
        break;
    }

    if (cap != null) {
      mStrokePaint.setStrokeCap(cap);
      mTextFillPaint.setStrokeCap(cap);
    }
  }

  public void setLineJoin(String lineJoin) {
    Paint.Join join = null;
    switch (lineJoin) {
      case "bevel":
        join = Paint.Join.BEVEL;
        break;
      case "round":
        join = Paint.Join.ROUND;
        break;
      case "miter":
        join = Paint.Join.MITER;
        break;
    }

    if (join != null) {
      mStrokePaint.setStrokeJoin(join);
      mTextFillPaint.setStrokeJoin(join);
    }
  }

  public void setMiterLimit(final float miterLimit) {
    mStrokePaint.setStrokeMiter(miterLimit);
    mTextFillPaint.setStrokeMiter(miterLimit);
  }

  public void setTextAlign(final String textAlign) {
    switch (textAlign) {
      case "left":
        mTextFillPaint.setTextAlign(Paint.Align.LEFT);
        break;
      case "center":
        mTextFillPaint.setTextAlign(Paint.Align.CENTER);
        break;
      case "right":
        mTextFillPaint.setTextAlign(Paint.Align.RIGHT);
        break;
    }
  }

  public void clear() {
    mCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
    mPath.reset();
  }

  public void clearRect(float x, float y, float width, float height) {
    mCanvas.drawRect(dpToPx(x), dpToPx(y), dpToPx(x + width), dpToPx(y + height), mClearPaint);
  }

  public void strokeRect(float x, float y, float width, float height) {
    mCanvas.drawRect(dpToPx(x), dpToPx(y), dpToPx(x + width), dpToPx(y + height), mStrokePaint);
  }

  public void fillRect(float x, float y, float width, float height) {
    mFillPaint.setStyle(Paint.Style.FILL);
    mCanvas.drawRect(dpToPx(x), dpToPx(y), dpToPx(x + width), dpToPx(y + height), mFillPaint);
  }

  public void beginPath() {
    mPath.reset();
  }

  public void closePath() {
    mPath.close();
  }

  public void stroke() {
    mCanvas.drawPath(mPath, mStrokePaint);
  }

  public void fill() {
    mCanvas.drawPath(mPath, mFillPaint);
  }

  public void arc(
      float x, float y, float radius, float startAngle, float endAngle, boolean counterclockwise) {
    RectF rect =
        new RectF(dpToPx(x - radius), dpToPx(y - radius), dpToPx(x + radius), dpToPx(y + radius));

    float PI2 = (float) Math.PI * 2;

    float sweepAngle = endAngle - startAngle;
    float initialAngle = startAngle % PI2;

    if (!counterclockwise && sweepAngle < 0) {
      sweepAngle %= PI2;
      if (sweepAngle < 0 || initialAngle == 0) {
        sweepAngle += PI2;
      }
    } else if (counterclockwise && sweepAngle > 0) {
      sweepAngle %= PI2;
      if (sweepAngle > 0 || initialAngle == 0) {
        sweepAngle -= PI2;
      }
    }

    mPath.addArc(rect, radiansToDegrees(initialAngle), radiansToDegrees(sweepAngle));
  }

  public void rect(float x, float y, float width, float height) {
    mPath.addRect(dpToPx(x), dpToPx(y), dpToPx(x + width), dpToPx(y + height), Path.Direction.CW);
  }

  public void lineTo(float x, float y) {
    mPath.lineTo(dpToPx(x), dpToPx(y));
  }

  public void moveTo(float x, float y) {
    mPath.moveTo(dpToPx(x), dpToPx(y));
  }

  public void drawCircle(float x, float y, float radius) {
    mCanvas.drawCircle(dpToPx(x), dpToPx(y), dpToPx(radius), mStrokePaint);
  }

  public void fillCircle(float x, float y, float radius) {
    mCanvas.drawCircle(dpToPx(x), dpToPx(y), dpToPx(radius), mFillPaint);
  }

  public void drawImage(Bitmap bitmap, float dx, float dy) {
    Matrix matrix = new Matrix();
    matrix.postScale(mPixelDensity, mPixelDensity, 1.0f, 1.0f);
    matrix.postTranslate(dpToPx(dx), dpToPx(dy));
    mCanvas.drawBitmap(bitmap, matrix, null);
  }

  public void drawImage(Bitmap bitmap, float dx, float dy, float dWidth, float dHeight) {
    int sWidth = bitmap.getWidth();
    int sHeight = bitmap.getHeight();
    float scaleX = dpToPx(dWidth) / sWidth;
    float scaleY = dpToPx(dHeight) / sHeight;

    Matrix matrix = new Matrix();
    matrix.postScale(scaleX, scaleY);
    matrix.postTranslate(dpToPx(dx), dpToPx(dy));
    mCanvas.drawBitmap(bitmap, matrix, null);
  }

  public void drawImage(
      Bitmap bitmap,
      float sx,
      float sy,
      float sWidth,
      float sHeight,
      float dx,
      float dy,
      float dWidth,
      float dHeight) {
    // Extract source x,y,width,height from original bitmap into destination bitmap
    Bitmap destBitmap =
        Bitmap.createBitmap(bitmap, (int) sx, (int) sy, (int) sWidth, (int) sHeight);

    float scaleX = dpToPx(dWidth) / sWidth;
    float scaleY = dpToPx(dHeight) / sHeight;

    Matrix matrix = new Matrix();
    matrix.postScale(scaleX, scaleY);
    matrix.postTranslate(dpToPx(dx), dpToPx(dy));
    mCanvas.drawBitmap(destBitmap, matrix, null);
  }

  public void setFont(final ReadableMap font) {
    String textSizeString = font.getString("fontSize");
    int textSize = Integer.parseInt(textSizeString.substring(0, textSizeString.length() - 2));
    mTextFillPaint.setTextSize(dpToPx(textSize));

    String fontFamily = font.getArray("fontFamily").getString(0);
    Typeface typeface = Typeface.DEFAULT;
    switch (fontFamily) {
      case "serif":
        typeface = Typeface.SERIF;
        break;
      case "sans-serif":
        typeface = Typeface.SANS_SERIF;
        break;
      case "monospace":
        typeface = Typeface.MONOSPACE;
        break;
    }

    int typefaceStyle = Typeface.NORMAL;

    String fontWeight = font.getString("fontWeight");
    if ("bold".equals(fontWeight)) {
      typefaceStyle += Typeface.BOLD;
    }

    String fontStyle = font.getString("fontStyle");
    if ("italic".equals(fontStyle)) {
      typefaceStyle += Typeface.ITALIC;
    }

    Typeface newTypeface = Typeface.create(typeface, typefaceStyle);
    mTextFillPaint.setTypeface(newTypeface);
  }

  public void fillText(String text, float x, float y) {
    mTextFillPaint.setStyle(Paint.Style.FILL);
    mCanvas.drawText(text, dpToPx(x), dpToPx(y), mTextFillPaint);
  }

  public void strokeText(String text, float x, float y) {
    mTextFillPaint.setStyle(Paint.Style.STROKE);
    mCanvas.drawText(text, dpToPx(x), dpToPx(y), mTextFillPaint);
  }

  /**
   * The CanvasRenderingContext2D.setTransform() method of the Canvas 2D API resets (overrides) the
   * current transformation to the identity matrix, and then invokes a transformation described by
   * the arguments of this method. This lets you scale, rotate, translate (move), and skew the
   * context.
   *
   * <p>The transformation matrix is described by:
   *
   * <p>a & c & e
   *
   * <p>b & d & f
   *
   * <p>0 & 0 & 1
   *
   * <p>setTransform() has two types of parameter that it can accept. The older type consists of
   * several parameters representing the individual components of the transformation matrix to set:
   *
   * @param a (m11) Horizontal scaling. A value of 1 results in no scaling.
   * @param b (m12) Vertical skewing.
   * @param c (m21) Horizontal skewing.
   * @param d (m22) Vertical scaling. A value of 1 results in no scaling.
   * @param e (dx) Horizontal translation (moving).
   * @param f (dy)Vertical translation (moving).
   *     <p>{@link
   *     https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D/setTransform}
   */
  protected void setTransform(float a, float b, float c, float d, float e, float f) {
    Matrix matrix = new Matrix();
    matrix.setValues(new float[] {a, c, dpToPx(e), b, d, dpToPx(f), 0, 0, 1});
    mCanvas.setMatrix(matrix);
  }

  protected void scale(float x, float y) {
    mCanvas.scale(x, y);
  }

  protected void rotate(float angle) {
    mCanvas.rotate(radiansToDegrees(angle));
  }

  protected void rotate(float angle, float x, float y) {
    mCanvas.rotate(radiansToDegrees(angle), dpToPx(x), dpToPx(y));
  }

  protected void translate(float x, float y) {
    mCanvas.translate(dpToPx(x), dpToPx(y));
  }

  /**
   * The CanvasRenderingContext2D save function saves the following states:
   *
   * <p>The drawing state that gets saved onto a stack consists of:
   *
   * <p>* The current transformation matrix. * The current clipping region. * The current dash list.
   * * The current values of the following attributes: strokeStyle, fillStyle, globalAlpha,
   * lineWidth, lineCap, lineJoin, miterLimit, lineDashOffset, shadowOffsetX, shadowOffsetY,
   * shadowBlur, shadowColor, globalCompositeOperation, font, textAlign, textBaseline, direction,
   * imageSmoothingEnabled.
   *
   * <p>However, the Android Canvas only saves the canvas, therefore the implementation needs to
   * additionally copy the current paints etc. into a canvas state that can be loaded on restore.
   *
   * <p>{@link https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D/save}
   */
  protected void save() {
    CanvasState savedState = new CanvasState(mStrokePaint, mFillPaint);
    mCanvas.save();
    mSavedStates.push(savedState);
    // Reset paint to initial values
    initPaint();
  }

  protected void restore() {
    if (!mSavedStates.empty()) {
      CanvasState canvasState = mSavedStates.pop();
      mStrokePaint = canvasState.mStrokePaint;
      mFillPaint = canvasState.mFillPaint;
      mCanvas.restore();
    }
  }

  public void invalidate() {
    mDrawingCanvasView.invalidate();
  }

  private float radiansToDegrees(float radians) {
    return (float) (radians * 180 / Math.PI);
  }

  private float dpToPx(float px) {
    return px * mPixelDensity;
  }

  static class CanvasState {
    Paint mStrokePaint;
    Paint mFillPaint;

    private CanvasState(Paint strokePaint, Paint fillPaint) {
      mStrokePaint = strokePaint;
      mFillPaint = fillPaint;
    }
  }
}
