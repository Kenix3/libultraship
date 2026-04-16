#pragma once

#include <memory>

#include "ship/window/Window.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Ship {

/**
 * @brief Holds the current horizontal and vertical scroll-wheel directions.
 */
struct WheelDirections {
    WheelDirection x; ///< Horizontal scroll direction.
    WheelDirection y; ///< Vertical scroll direction.
};

/**
 * @brief Singleton that tracks mouse scroll-wheel state and converts it into
 *        axis-like values consumable by the input mapping system.
 *
 * Call Update() once per frame to sample the latest wheel data from the
 * window back-end. The resulting direction values and buffered values can
 * then be queried by MouseWheelToAxisDirectionMapping and related classes.
 */
class WheelHandler {
  public:
    /** @brief Constructs a WheelHandler with default (zero) state. */
    WheelHandler();

    /** @brief Destructor. */
    ~WheelHandler();

    /**
     * @brief Returns the global WheelHandler singleton.
     * @return Shared pointer to the WheelHandler instance.
     */
    static std::shared_ptr<WheelHandler> GetInstance();

    /** @brief Samples the latest wheel data from the window back-end. */
    void Update();

    /**
     * @brief Returns the raw scroll-wheel coordinates for the current frame.
     * @return A CoordsF with x (horizontal) and y (vertical) scroll amounts.
     */
    CoordsF GetCoords();

    /**
     * @brief Returns the current horizontal and vertical scroll directions.
     * @return A WheelDirections struct.
     */
    WheelDirections GetDirections();

    /**
     * @brief Returns the instantaneous direction value for the given wheel direction.
     * @param direction The wheel direction to query.
     * @return The direction magnitude as a float.
     */
    float GetDirectionValue(WheelDirection direction);

    /**
     * @brief Returns the buffered (smoothed) direction value for the given wheel direction.
     * @param direction The wheel direction to query.
     * @return The buffered direction magnitude as a float.
     */
    float GetBufferedDirectionValue(WheelDirection direction);

  private:
    float CalcDirectionValue(CoordsF& coords, WheelDirection direction);
    void UpdateAxisBuffer(float* buf, float input);

    static std::shared_ptr<WheelHandler> mInstance;

    WheelDirections mDirections;
    CoordsF mCoords;
    CoordsF mBufferedCoords;
};
} // namespace Ship
