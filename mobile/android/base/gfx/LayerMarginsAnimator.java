/* -*- Mode: Java; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.TouchEventInterceptor;
import org.mozilla.gecko.util.FloatUtils;

import android.graphics.PointF;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.Log;
import android.view.animation.DecelerateInterpolator;
import android.view.MotionEvent;
import android.view.View;

import java.util.Timer;
import java.util.TimerTask;

public class LayerMarginsAnimator implements TouchEventInterceptor {
    private static final String LOGTAG = "GeckoLayerMarginsAnimator";
    private static final float MS_PER_FRAME = 1000.0f / 60.0f;
    private static final long MARGIN_ANIMATION_DURATION = 250;

    /* This is the proportion of the viewport rect, minus maximum margins,
     * that should be used to be able to expose margins. This does not apply
     * for hiding margins.
     */
    private static final float SHOW_MARGINS_AREA = 0.25f;

    /* This rect stores the maximum value margins can grow to when scrolling */
    private final RectF mMaxMargins;
    /* If this boolean is true, scroll changes will not affect margins */
    private boolean mMarginsPinned;
    /* The timer that handles showing/hiding margins */
    private Timer mAnimationTimer;
    /* This interpolator is used for the above mentioned animation */
    private final DecelerateInterpolator mInterpolator;
    /* The GeckoLayerClient whose margins will be animated */
    private final GeckoLayerClient mTarget;
    /* The position of the first touch event */
    private final PointF mTouchStartPosition;

    public LayerMarginsAnimator(GeckoLayerClient aTarget, LayerView aView) {
        // Assign member variables from parameters
        mTarget = aTarget;

        // Create other member variables
        mMaxMargins = new RectF();
        mInterpolator = new DecelerateInterpolator();
        mTouchStartPosition = new PointF();

        // Listen to touch events, for auto-pinning
        aView.addTouchInterceptor(this);
    }

    /**
     * Sets the maximum values for margins to grow to, in pixels.
     */
    public synchronized void setMaxMargins(float left, float top, float right, float bottom) {
        mMaxMargins.set(left, top, right, bottom);

        // Update the Gecko-side global for fixed viewport margins.
        GeckoAppShell.sendEventToGecko(
            GeckoEvent.createBroadcastEvent("Viewport:FixedMarginsChanged",
                "{ \"top\" : " + top + ", \"right\" : " + right
                + ", \"bottom\" : " + bottom + ", \"left\" : " + left + " }"));
    }

    private void animateMargins(final float left, final float top, final float right, final float bottom, boolean immediately) {
        if (mAnimationTimer != null) {
            mAnimationTimer.cancel();
            mAnimationTimer = null;
        }

        if (immediately) {
            ImmutableViewportMetrics newMetrics = mTarget.getViewportMetrics().setMargins(left, top, right, bottom);
            mTarget.forceViewportMetrics(newMetrics, true, true);
            return;
        }

        ImmutableViewportMetrics metrics = mTarget.getViewportMetrics();

        final long startTime = SystemClock.uptimeMillis();
        final float startLeft = metrics.marginLeft;
        final float startTop = metrics.marginTop;
        final float startRight = metrics.marginRight;
        final float startBottom = metrics.marginBottom;

        mAnimationTimer = new Timer("Margin Animation Timer");
        mAnimationTimer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                float progress = mInterpolator.getInterpolation(
                    Math.min(1.0f, (SystemClock.uptimeMillis() - startTime)
                                     / (float)MARGIN_ANIMATION_DURATION));

                synchronized(mTarget.getLock()) {
                    ImmutableViewportMetrics oldMetrics = mTarget.getViewportMetrics();
                    ImmutableViewportMetrics newMetrics = oldMetrics.setMargins(
                        FloatUtils.interpolate(startLeft, left, progress),
                        FloatUtils.interpolate(startTop, top, progress),
                        FloatUtils.interpolate(startRight, right, progress),
                        FloatUtils.interpolate(startBottom, bottom, progress));
                    PointF oldOffset = oldMetrics.getMarginOffset();
                    PointF newOffset = newMetrics.getMarginOffset();
                    newMetrics =
                        newMetrics.offsetViewportByAndClamp(newOffset.x - oldOffset.x,
                                                            newOffset.y - oldOffset.y);

                    if (progress >= 1.0f) {
                        if (mAnimationTimer != null) {
                            mAnimationTimer.cancel();
                            mAnimationTimer = null;
                        }

                        // Force a redraw and update Gecko
                        mTarget.forceViewportMetrics(newMetrics, true, true);
                    } else {
                        mTarget.forceViewportMetrics(newMetrics, false, false);
                    }
                }
            }
        }, 0, (int)MS_PER_FRAME);
    }

    /**
     * Exposes the margin area by growing the margin components of the current
     * metrics to the values set in setMaxMargins.
     */
    public synchronized void showMargins(boolean immediately) {
        animateMargins(mMaxMargins.left, mMaxMargins.top, mMaxMargins.right, mMaxMargins.bottom, immediately);
    }

    public synchronized void hideMargins(boolean immediately) {
        animateMargins(0, 0, 0, 0, immediately);
    }

    public void setMarginsPinned(boolean pin) {
        if (pin == mMarginsPinned) {
            return;
        }

        mMarginsPinned = pin;
    }

    /**
     * This function will scroll a margin down to zero, or up to the maximum
     * specified margin size and return the left-over delta.
     * aMargins are in/out parameters. In specifies the current margin size,
     * and out specifies the modified margin size. They are specified in the
     * order of start-margin, then end-margin.
     * This function will also take into account what touch initiated this
     * scroll and react accordingly. If a scroll was initiated from a point
     * outisde of the active area of the viewport, margins can only be hidden
     * and not shown.
     * aNegativeOffset can be used if the remaining delta should be determined
     * by the end-margin instead of the start-margin (for example, in rtl
     * pages).
     */
    private float scrollMargin(float[] aMargins, float aDelta,
                               float aOverscrollStart, float aOverscrollEnd,
                               float aTouchCoordinate,
                               float aViewportStart, float aViewportEnd,
                               float aPageStart, float aPageEnd,
                               float aMaxMarginStart, float aMaxMarginEnd,
                               boolean aNegativeOffset) {
        float marginStart = aMargins[0];
        float marginEnd = aMargins[1];
        float viewportSize = aViewportEnd - aViewportStart;
        float activeArea = viewportSize * SHOW_MARGINS_AREA;

        if (aDelta >= 0) {
            float marginDelta = Math.max(0, aDelta - aOverscrollStart);
            aMargins[0] = marginStart - Math.min(marginDelta, marginStart);
            if (aTouchCoordinate < viewportSize - activeArea) {
                  // In the situation that the touch started outside of the
                  // active area for exposing this margin, we only want the
                  // margin to be exposed when reaching the extremity of the
                  // page.
                marginDelta = Math.max(0, marginDelta - (aPageEnd - aViewportEnd));
            }
            aMargins[1] = marginEnd + Math.min(marginDelta, aMaxMarginEnd - marginEnd);
        } else {
            float marginDelta = Math.max(0, -aDelta - aOverscrollEnd);
            aMargins[1] = marginEnd - Math.min(marginDelta, marginEnd);
            if (aTouchCoordinate > activeArea) {
                marginDelta = Math.max(0, marginDelta - (aViewportStart - aPageStart));
            }
            aMargins[0] = marginStart + Math.min(marginDelta, aMaxMarginStart - marginStart);
        }

        if (aNegativeOffset) {
            return aDelta - (marginEnd - aMargins[1]);
        }
        return aDelta - (marginStart - aMargins[0]);
    }

    /*
     * Taking maximum margins into account, offsets the margins and then the
     * viewport origin and returns the modified metrics.
     */
    ImmutableViewportMetrics scrollBy(ImmutableViewportMetrics aMetrics, float aDx, float aDy) {
        // Make sure to cancel any margin animations when scrolling begins
        if (mAnimationTimer != null) {
            mAnimationTimer.cancel();
            mAnimationTimer = null;
        }

        float[] newMarginsX = { aMetrics.marginLeft, aMetrics.marginRight };
        float[] newMarginsY = { aMetrics.marginTop, aMetrics.marginBottom };

        // Only alter margins if the toolbar isn't pinned
        if (!mMarginsPinned) {
            RectF overscroll = aMetrics.getOverscroll();
            // Only allow margins to scroll if the page can fill the viewport.
            if (aMetrics.getPageWidth() >= aMetrics.getWidth()) {
                aDx = scrollMargin(newMarginsX, aDx,
                                   overscroll.left, overscroll.right,
                                   mTouchStartPosition.x,
                                   aMetrics.viewportRectLeft, aMetrics.viewportRectRight,
                                   aMetrics.pageRectLeft, aMetrics.pageRectRight,
                                   mMaxMargins.left, mMaxMargins.right,
                                   aMetrics.isRTL);
            }
            if (aMetrics.getPageHeight() >= aMetrics.getHeight()) {
                aDy = scrollMargin(newMarginsY, aDy,
                                   overscroll.top, overscroll.bottom,
                                   mTouchStartPosition.y,
                                   aMetrics.viewportRectTop, aMetrics.viewportRectBottom,
                                   aMetrics.pageRectTop, aMetrics.pageRectBottom,
                                   mMaxMargins.top, mMaxMargins.bottom,
                                   false);
            }
        }

        return aMetrics.setMargins(newMarginsX[0], newMarginsY[0], newMarginsX[1], newMarginsY[1]).offsetViewportBy(aDx, aDy);
    }

    /** Implementation of TouchEventInterceptor */
    @Override
    public boolean onTouch(View view, MotionEvent event) {
        return false;
    }

    /** Implementation of TouchEventInterceptor */
    @Override
    public boolean onInterceptTouchEvent(View view, MotionEvent event) {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN && event.getPointerCount() == 1) {
            mTouchStartPosition.set(event.getX(), event.getY());
        }

        return false;
    }
}
