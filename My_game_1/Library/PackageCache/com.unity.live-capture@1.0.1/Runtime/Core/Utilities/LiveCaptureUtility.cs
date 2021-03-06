using System.Linq;
using System.Collections.Generic;
using UnityEngine;

namespace Unity.LiveCapture
{
    /// <summary>
    /// Contains useful extension methods.
    /// </summary>
    static class LiveCaptureUtility
    {
        /// <summary>
        /// Appends an item to a list if the item is not already contained by the list.
        /// </summary>
        /// <param name="list">The list to append to.</param>
        /// <param name="item">The item to add.</param>
        /// <typeparam name="T">The list element type.</typeparam>
        /// <returns>True if a new item was added; false otherwise.</returns>
        public static bool AddUnique<T>(this List<T> list, T item)
        {
            if (!list.Contains(item))
            {
                list.Add(item);
                return true;
            }

            return false;
        }

        /// <summary>
        /// Checks if a <see cref="LiveCaptureDevice" /> is live and the associated <see cref="TakeRecorder" /> is enabled
        /// and has live mode set.
        /// </summary>
        /// <param name="device">The device to check.</param>
        /// <returns>True if a device is live and its associated take recorder is live and enabled; false otherwise.</returns>
        public static bool IsLiveActive(this LiveCaptureDevice device)
        {
            var takeRecorder = device.GetTakeRecorder() as ITakeRecorderInternal;

            if (takeRecorder == null)
            {
                return false;
            }

            return takeRecorder.IsEnabled
                && takeRecorder.IsLive()
                && device.isActiveAndEnabled
                && device.IsLive();
        }

        /// <summary>
        /// Returns the camera that has the higher depth.
        /// </summary>
        /// <returns>The camera that has the higher depth, if any.</returns>
        public static Camera GetTopCamera()
        {
            return Camera.allCameras.OrderByDescending(c => c.depth).FirstOrDefault();
        }
    }
}
