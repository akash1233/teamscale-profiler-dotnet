﻿using System;

namespace UploadDaemon.Archiving
{
    /// <summary>
    /// A provider that supplies the current date time.
    /// </summary>
    public interface IDateTimeProvider
    {
        /// <summary>
        /// Current date time.
        /// </summary>
        DateTime Now { get; }
    }
}
