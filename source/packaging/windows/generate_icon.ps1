param(
    [Parameter(Mandatory = $true)]
    [string]$InputPng,

    [Parameter(Mandatory = $true)]
    [string]$OutputIco
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing

$inputPath = [System.IO.Path]::GetFullPath($InputPng)
$outputPath = [System.IO.Path]::GetFullPath($OutputIco)
$outputDir = [System.IO.Path]::GetDirectoryName($outputPath)
if (-not [string]::IsNullOrEmpty($outputDir)) {
    [System.IO.Directory]::CreateDirectory($outputDir) | Out-Null
}

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$frames = New-Object System.Collections.Generic.List[object]

$sourceImage = [System.Drawing.Image]::FromFile($inputPath)
try {
    foreach ($size in $sizes) {
        $bitmap = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.Clear([System.Drawing.Color]::Transparent)
                $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $graphics.DrawImage($sourceImage, 0, 0, $size, $size)
            }
            finally {
                $graphics.Dispose()
            }

            $stream = New-Object System.IO.MemoryStream
            try {
                $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
                $frames.Add([PSCustomObject]@{
                    Size = $size
                    Bytes = $stream.ToArray()
                }) | Out-Null
            }
            finally {
                $stream.Dispose()
            }
        }
        finally {
            $bitmap.Dispose()
        }
    }
}
finally {
    $sourceImage.Dispose()
}

$file = [System.IO.File]::Open($outputPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
try {
    $writer = New-Object System.IO.BinaryWriter($file)
    try {
        $writer.Write([UInt16]0)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]$frames.Count)

        $offset = 6 + (16 * $frames.Count)
        foreach ($frame in $frames) {
            $dimension = if ($frame.Size -ge 256) { 0 } else { [byte]$frame.Size }
            $writer.Write([byte]$dimension)
            $writer.Write([byte]$dimension)
            $writer.Write([byte]0)
            $writer.Write([byte]0)
            $writer.Write([UInt16]1)
            $writer.Write([UInt16]32)
            $writer.Write([UInt32]$frame.Bytes.Length)
            $writer.Write([UInt32]$offset)
            $offset += $frame.Bytes.Length
        }

        foreach ($frame in $frames) {
            $writer.Write([byte[]]$frame.Bytes)
        }
    }
    finally {
        $writer.Dispose()
    }
}
finally {
    $file.Dispose()
}
