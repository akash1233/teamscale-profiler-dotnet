﻿using NUnit.Framework;
using UploadDaemon.SymbolAnalysis;
using UploadDaemon;
using System;
using System.IO;
using System.Collections.Generic;

using System.IO.Abstractions;
using Moq;
using Cqse.ConQAT.Dotnet.Bummer;

[TestFixture]
public class LineCoverageSynthesizerTest
{
    // 100663427 corresponds to MainViewModel#get_SelectedBitnessIndex in ProfilerGUI.pdb
    // obtained with cvdump.exe
    private static readonly uint ExistingMethodToken = 100663427;

    [Test]
    public void TestSynthesizing()
    {
        ParsedTraceFile traceFile = new ParsedTraceFile(new string[] {
            "Assembly=ProfilerGUI:2 Version:1.0.0.0",
            $"Inlined=2:{ExistingMethodToken}",
        }, "coverage_12345_1234.txt");
        string coverageReport = new LineCoverageSynthesizer().ConvertToLineCoverageReport(traceFile, TestUtils.TestDataDirectory,
            new Common.GlobPatternList(new List<string> { "*" }, new List<string> { }));

        Assert.That(NormalizeNewLines(coverageReport.Trim()), Is.EqualTo(NormalizeNewLines(@"# isMethodAccurate=true
\\VBOXSVR\proj\teamscale-profiler-dotnet\ProfilerGUI\Source\Configurator\MainViewModel.cs
37-39")));
    }

    [Test]
    public void TracesWithoutCoverageShouldResultInNullBeingReturned()
    {
        ParsedTraceFile traceFile = new ParsedTraceFile(new string[] {
            "Assembly=ProfilerGUI:2 Version:1.0.0.0",
        }, "coverage_12345_1234.txt");

        string report = new LineCoverageSynthesizer().ConvertToLineCoverageReport(traceFile, TestUtils.TestDataDirectory,
                new Common.GlobPatternList(new List<string> { "*" }, new List<string> { }));
        Assert.That(report, Is.Null);
    }

    [Test]
    public void ExcludingAllPdbsShouldResultInException()
    {
        ParsedTraceFile traceFile = new ParsedTraceFile(new string[] {
            "Assembly=ProfilerGUI:2 Version:1.0.0.0",
            $"Inlined=2:{ExistingMethodToken}",
        }, "coverage_12345_1234.txt");

        Exception exception = Assert.Throws<LineCoverageSynthesizer.LineCoverageConversionFailedException>(() =>
        {
            new LineCoverageSynthesizer().ConvertToLineCoverageReport(traceFile, TestUtils.TestDataDirectory,
                new Common.GlobPatternList(new List<string> { "xx" }, new List<string> { "*" }));
        });

        Assert.That(exception.Message, Contains.Substring("no symbols"));
    }

    [Test]
    public void CompilerHiddenLinesShouldBeIgnored()
    {
        ParsedTraceFile traceFile = new ParsedTraceFile(new string[] {
            "Assembly=Test:2 Version:1.0.0.0",
            "Inlined=2:1234",
        }, "coverage_12345_1234.txt");

        AssemblyMethodMappings mappings = new AssemblyMethodMappings
        {
            AssemblyName = "Test",
            SymbolFileName = "Test.pdb",
        };
        mappings.MethodMappings.Add(new MethodMapping
        {
            MethodToken = 1234,
            SourceFile = "",
            StartLine = 16707566,
            EndLine = 16707566,
        });
        mappings.MethodMappings.Add(new MethodMapping
        {
            MethodToken = 1234,
            SourceFile = @"c:\some\file.cs",
            StartLine = 16707566,
            EndLine = 16707566,
        });

        SymbolCollection symbolCollection = new SymbolCollection(new List<AssemblyMethodMappings>() { mappings });

        Dictionary<string, LineCoverageSynthesizer.FileCoverage> coverage = LineCoverageSynthesizer.ConvertToLineCoverage(traceFile, symbolCollection, TestUtils.TestDataDirectory,
            new Common.GlobPatternList(new List<string> { "*" }, new List<string> { }));

        Assert.That(coverage, Is.Empty);
    }

    private string NormalizeNewLines(string text)
    {
        return text.Replace("\r\n", "\n").Replace("\r", "\n");
    }
}