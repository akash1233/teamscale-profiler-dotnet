﻿using System;
using System.Globalization;
using UploadDaemon.Configuration;

namespace UploadDaemon.Upload
{
    /// <summary>
    /// Formats the message for an upload commit.
    /// </summary>
    public class MessageFormatter
    {
        private readonly TeamscaleServer server;

        public MessageFormatter(TeamscaleServer server)
        {
            this.server = server;
        }

        /// <summary>
        /// Formats the configured message template by replacing all placeholders with actual values.
        /// </summary>
        /// <param name="assemblyVersion">The version read from the version assembly</param>
        /// <returns></returns>
        public string Format(string assemblyVersion)
        {
            string formattedTime = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture);
            return server.Message.Replace("%v", assemblyVersion).Replace("%p", server.Partition).Replace("%t", formattedTime);
        }
    }
}