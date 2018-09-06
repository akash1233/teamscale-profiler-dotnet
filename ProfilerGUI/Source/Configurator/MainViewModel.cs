﻿using NLog;
using ProfilerGUI.Source.Runner;
using ProfilerGUI.Source.Shared;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;

namespace ProfilerGUI.Source.Configurator
{
    /// <summary>
    /// The main view model to interact with the <see cref="MainWindow"/>.
    /// </summary>
    public class MainViewModel : INotifyPropertyChanged
    {
        private static readonly Logger logger = LogManager.GetCurrentClassLogger();

        /// <summary> <inheritDoc /> </summary>
        public event PropertyChangedEventHandler PropertyChanged;

        /// <summary>
        /// Model for the application to profile.
        /// </summary>
        public TargetAppModel TargetApp { get; private set; }

        /// <summary>
        /// Index of the selected bitness (32bit vs 64bit).
        /// </summary>
        public int SelectedBitnessIndex
        {
            get
            {
                return (int)TargetApp.ApplicationType;
            }
            set
            {
                TargetApp.ApplicationType = (EApplicationType)value;
                PropertyChanged.Raise(this);
            }
        }

        private readonly UploadConfigWindow uploadConfigWindow = new UploadConfigWindow();

        /// <summary>
        /// Summary of the configured upload.
        /// </summary>
        public string UploadSummary
        {
            get
            {
                if (uploadConfigWindow.Config == null)
                {
                    return "No upload";
                }
                if (uploadConfigWindow.Config.Teamscale != null)
                {
                    return $"Upload to {uploadConfigWindow.Config.Teamscale}";
                }
                return $"Upload to directory {uploadConfigWindow.Config.Directory}";
            }
        }

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="launchTargetAppDirectly">If this is set, then the target application is started right away.
        /// A default config must exist for this to do anything.</param>
        public MainViewModel(bool launchTargetAppDirectly)
        {
            ProfilerConfiguration configuration = new ProfilerConfiguration();
            if (File.Exists(ProfilerConfiguration.ConfigFilePath))
            {
                logger.Info("Loaded config from {configPath}", ProfilerConfiguration.ConfigFilePath);
                configuration = ProfilerConfiguration.ReadFromFile();

                if (launchTargetAppDirectly)
                {
                    RunProfiledApplication();
                }
            }

            TargetApp = new TargetAppModel(configuration);
            TargetApp.PropertyChanged += (sender, args) =>
            {
                PropertyChanged.ReRaise(this, nameof(TargetApp), args);

                if (args.PropertyName == nameof(TargetApp.ApplicationType))
                {
                    PropertyChanged.Raise(this, nameof(SelectedBitnessIndex));
                }
            };
        }

        /// <summary>
        /// Shows the dialog to configure the trace upload.
        /// </summary>
        internal void OpenUploadConfigDialog()
        {
            uploadConfigWindow.ShowDialog();
            PropertyChanged.Raise(this, nameof(UploadSummary));
        }

        /// <summary>
        /// Resets the fields that describe the application that should be profiled.
        /// </summary>
        internal void ClearTargetAppFields()
        {
            TargetApp.Clear();
        }

        /// <summary>
        /// Saves the current configuration to a file.
        /// </summary>
        internal void SaveConfiguration()
        {
            try
            {
                TargetApp.Configuration.WriteToFile();
                logger.Info("Wrote config to {configPath}", ProfilerConfiguration.ConfigFilePath);
            }
            catch (Exception e)
            {
                logger.Error(e, "Could not save configuration to {configPath}", ProfilerConfiguration.ConfigFilePath);
            }
        }

        /// <summary>
        /// Starts the profiled application with the profiler attached.
        /// </summary>
        internal void RunProfiledApplication()
        {
            if (string.IsNullOrEmpty(TargetApp.ApplicationPath))
            {
                logger.Error("You did not configure an application to profile");
                return;
            }

            ProfilerRunner runner = new ProfilerRunner(TargetApp.Configuration);
            runner.RunAsynchronously();
        }
    }
}