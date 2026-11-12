param(
    [switch]$NoOverwrite
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$pptDir = Join-Path $root "ppt"
$previewDir = Join-Path $root "previews"
$zipFile = Join-Path $root "serial_screen_tjc_native_ppt_v5.zip"
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$backupDir = Join-Path $pptDir "backup_before_clean_$stamp"

if (-not (Test-Path $pptDir)) { New-Item -ItemType Directory -Path $pptDir | Out-Null }
if (-not (Test-Path $previewDir)) { throw "Missing preview directory: $previewDir" }

function Xml-Escape([string]$value) {
    return [System.Security.SecurityElement]::Escape($value)
}

function RelXml($items) {
    $body = ($items | ForEach-Object {
        '<Relationship Id="{0}" Type="{1}" Target="{2}"/>' -f (Xml-Escape $_.Id), (Xml-Escape $_.Type), (Xml-Escape $_.Target)
    }) -join ""
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">{0}</Relationships>' -f $body
}

function ContentTypesXml([int]$slideCount) {
    $overrides = @()
    $overrides += '<Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>'
    $overrides += '<Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/>'
    $overrides += '<Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/>'
    $overrides += '<Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>'
    $overrides += '<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>'
    $overrides += '<Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>'
    for ($i = 1; $i -le $slideCount; $i++) {
        $overrides += '<Override PartName="/ppt/slides/slide{0}.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>' -f $i
    }
    @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="png" ContentType="image/png"/>
  $($overrides -join "`n  ")
</Types>
"@
}

function PresentationXml([int]$slideCount) {
    $sldIds = @()
    for ($i = 1; $i -le $slideCount; $i++) {
        $sldIds += '<p:sldId id="{0}" r:id="rId{1}"/>' -f (255 + $i), $i
    }
    @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rIdMaster"/></p:sldMasterIdLst>
  <p:sldIdLst>$($sldIds -join "")</p:sldIdLst>
  <p:sldSz cx="7620000" cy="4572000" type="custom"/>
  <p:notesSz cx="6858000" cy="9144000"/>
  <p:defaultTextStyle>
    <a:defPPr><a:defRPr lang="zh-CN"/></a:defPPr>
  </p:defaultTextStyle>
</p:presentation>
"@
}

function PresentationRelsXml([int]$slideCount) {
    $items = @()
    for ($i = 1; $i -le $slideCount; $i++) {
        $items += @{ Id = "rId$i"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide"; Target = "slides/slide$i.xml" }
    }
    $items += @{ Id = "rIdMaster"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster"; Target = "slideMasters/slideMaster1.xml" }
    $items += @{ Id = "rIdTheme"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme"; Target = "theme/theme1.xml" }
    RelXml $items
}

function SlideXml([int]$index, [string]$name) {
    $safeName = Xml-Escape $name
    @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld name="$safeName">
    <p:spTree>
      <p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>
      <p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>
      <p:pic>
        <p:nvPicPr><p:cNvPr id="2" name="preview_$index"/><p:cNvPicPr><a:picLocks noChangeAspect="1"/></p:cNvPicPr><p:nvPr/></p:nvPicPr>
        <p:blipFill><a:blip r:embed="rIdImage"/><a:stretch><a:fillRect/></a:stretch></p:blipFill>
        <p:spPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="7620000" cy="4572000"/></a:xfrm><a:prstGeom prst="rect"><a:avLst/></a:prstGeom></p:spPr>
      </p:pic>
    </p:spTree>
  </p:cSld>
  <p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>
</p:sld>
"@
}

function SlideMasterXml {
    @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr/></p:spTree></p:cSld>
  <p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/>
  <p:sldLayoutIdLst><p:sldLayoutId id="2147483649" r:id="rIdLayout1"/></p:sldLayoutIdLst>
  <p:txStyles><p:titleStyle/><p:bodyStyle/><p:otherStyle/></p:txStyles>
</p:sldMaster>
"@
}

function SlideLayoutXml {
    @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldLayout xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" type="blank" preserve="1">
  <p:cSld name="Blank"><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr/></p:spTree></p:cSld>
  <p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>
</p:sldLayout>
"@
}

function ThemeXml {
    @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Clean">
  <a:themeElements>
    <a:clrScheme name="Clean"><a:dk1><a:srgbClr val="18231F"/></a:dk1><a:lt1><a:srgbClr val="FFFFFF"/></a:lt1><a:dk2><a:srgbClr val="344B5A"/></a:dk2><a:lt2><a:srgbClr val="F7F8F4"/></a:lt2><a:accent1><a:srgbClr val="168A6B"/></a:accent1><a:accent2><a:srgbClr val="486F8F"/></a:accent2><a:accent3><a:srgbClr val="D9892B"/></a:accent3><a:accent4><a:srgbClr val="C6544A"/></a:accent4><a:accent5><a:srgbClr val="66746D"/></a:accent5><a:accent6><a:srgbClr val="DCE4DE"/></a:accent6><a:hlink><a:srgbClr val="168A6B"/></a:hlink><a:folHlink><a:srgbClr val="0F6652"/></a:folHlink></a:clrScheme>
    <a:fontScheme name="Clean"><a:majorFont><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/><a:cs typeface="Microsoft YaHei"/></a:majorFont><a:minorFont><a:latin typeface="Microsoft YaHei"/><a:ea typeface="Microsoft YaHei"/><a:cs typeface="Microsoft YaHei"/></a:minorFont></a:fontScheme>
    <a:fmtScheme name="Clean"><a:fillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:fillStyleLst><a:lnStyleLst><a:ln w="9525"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:ln></a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst><a:bgFillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill></a:bgFillStyleLst></a:fmtScheme>
  </a:themeElements>
</a:theme>
"@
}

function AppXml([int]$slideCount) {
    @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">
  <Application>PowerPoint</Application><PresentationFormat>Custom</PresentationFormat><Slides>$slideCount</Slides><Notes>0</Notes><HiddenSlides>0</HiddenSlides><MMClips>0</MMClips><ScaleCrop>false</ScaleCrop><Company></Company><LinksUpToDate>false</LinksUpToDate><SharedDoc>false</SharedDoc><HyperlinksChanged>false</HyperlinksChanged><AppVersion>16.0000</AppVersion>
</Properties>
"@
}

function CoreXml {
    $now = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <dc:title>智能衣橱串口屏界面</dc:title><dc:creator>Codex</dc:creator><cp:lastModifiedBy>Codex</cp:lastModifiedBy><dcterms:created xsi:type="dcterms:W3CDTF">$now</dcterms:created><dcterms:modified xsi:type="dcterms:W3CDTF">$now</dcterms:modified>
</cp:coreProperties>
"@
}

function Write-ZipText($zip, [string]$path, [string]$text) {
    $entry = $zip.CreateEntry($path)
    $stream = $entry.Open()
    try {
        $writer = New-Object System.IO.StreamWriter($stream, [System.Text.UTF8Encoding]::new($false))
        try { $writer.Write($text) } finally { $writer.Dispose() }
    } finally {
        $stream.Dispose()
    }
}

function Add-ZipFile($zip, [string]$path, [string]$source) {
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $source, $path) | Out-Null
}

function New-FlatPptx([string]$prefix, [string]$outPath) {
    $images = Get-ChildItem -LiteralPath $previewDir -File -Filter "$prefix`_*.png" |
        Where-Object { $_.BaseName -match "^$prefix`_\d{2}_" } |
        Sort-Object Name
    if ($images.Count -eq 0) { throw "No preview PNG files found for prefix: $prefix" }

    if (Test-Path $outPath) { Remove-Item -LiteralPath $outPath -Force }
    $zip = [System.IO.Compression.ZipFile]::Open($outPath, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        Write-ZipText $zip "[Content_Types].xml" (ContentTypesXml $images.Count)
        Write-ZipText $zip "_rels/.rels" (RelXml @(
            @{ Id = "rId1"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"; Target = "ppt/presentation.xml" },
            @{ Id = "rId2"; Type = "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties"; Target = "docProps/core.xml" },
            @{ Id = "rId3"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties"; Target = "docProps/app.xml" }
        ))
        Write-ZipText $zip "docProps/core.xml" (CoreXml)
        Write-ZipText $zip "docProps/app.xml" (AppXml $images.Count)
        Write-ZipText $zip "ppt/presentation.xml" (PresentationXml $images.Count)
        Write-ZipText $zip "ppt/_rels/presentation.xml.rels" (PresentationRelsXml $images.Count)
        Write-ZipText $zip "ppt/slideMasters/slideMaster1.xml" (SlideMasterXml)
        Write-ZipText $zip "ppt/slideMasters/_rels/slideMaster1.xml.rels" (RelXml @(
            @{ Id = "rIdLayout1"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout"; Target = "../slideLayouts/slideLayout1.xml" },
            @{ Id = "rIdTheme"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme"; Target = "../theme/theme1.xml" }
        ))
        Write-ZipText $zip "ppt/slideLayouts/slideLayout1.xml" (SlideLayoutXml)
        Write-ZipText $zip "ppt/slideLayouts/_rels/slideLayout1.xml.rels" (RelXml @(
            @{ Id = "rIdMaster"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster"; Target = "../slideMasters/slideMaster1.xml" }
        ))
        Write-ZipText $zip "ppt/theme/theme1.xml" (ThemeXml)

        for ($i = 0; $i -lt $images.Count; $i++) {
            $n = $i + 1
            Write-ZipText $zip "ppt/slides/slide$n.xml" (SlideXml $n $images[$i].BaseName)
            Write-ZipText $zip "ppt/slides/_rels/slide$n.xml.rels" (RelXml @(
                @{ Id = "rIdImage"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"; Target = "../media/image$n.png" },
                @{ Id = "rIdLayout"; Type = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout"; Target = "../slideLayouts/slideLayout1.xml" }
            ))
            Add-ZipFile $zip "ppt/media/image$n.png" $images[$i].FullName
        }
    } finally {
        $zip.Dispose()
    }
    Write-Host "wrote $outPath"
}

$deckSpecs = @(
    @{ Prefix = "backgrounds"; Output = "serial_screen_user_backgrounds_v5.pptx" },
    @{ Prefix = "controls"; Output = "serial_screen_user_controls_plan_v5.pptx" }
)

New-Item -ItemType Directory -Path $backupDir | Out-Null
foreach ($spec in $deckSpecs) {
    $existing = Join-Path $pptDir $spec.Output
    if (Test-Path $existing) {
        Copy-Item -LiteralPath $existing -Destination (Join-Path $backupDir $spec.Output) -Force
    }
}
if (Test-Path $zipFile) {
    Copy-Item -LiteralPath $zipFile -Destination (Join-Path $backupDir "serial_screen_tjc_native_ppt_v5.zip") -Force
}

foreach ($spec in $deckSpecs) {
    $outPath = Join-Path $pptDir $spec.Output
    if ($NoOverwrite) {
        $outPath = Join-Path $pptDir ($spec.Output -replace "\.pptx$", "_clean_$stamp.pptx")
    }
    New-FlatPptx $spec.Prefix $outPath
}

$manifestFile = Join-Path $root "manifest_v5.json"
$pageMap = Join-Path $root "reference_tables\ppt_page_map.csv"
if (Test-Path $pageMap) {
    $pages = Import-Csv -LiteralPath $pageMap
} else {
    $pages = @()
}

$manifest = [ordered]@{
    version = "v5-clean-flat"
    slideSize = [ordered]@{ width = 800; height = 480 }
    generatedAt = (Get-Date).ToString("s")
    note = "Slides are rebuilt from verified PNG previews to avoid text encoding or font rendering issues."
    decks = [ordered]@{
        backgrounds = (Join-Path $pptDir "serial_screen_user_backgrounds_v5.pptx")
        controls = (Join-Path $pptDir "serial_screen_user_controls_plan_v5.pptx")
    }
    pages = @($pages | ForEach-Object {
        [ordered]@{
            no = [int]$_.slide_no
            name = $_.page_name
            title = $_.title
            note = $_.note
        }
    })
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestFile -Encoding utf8

if (Test-Path $zipFile) { Remove-Item -LiteralPath $zipFile -Force }
$items = @("ppt", "previews", "reference_tables", "reports", "manifest_v5.json") |
    ForEach-Object { Join-Path $root $_ } |
    Where-Object { Test-Path $_ }
Compress-Archive -LiteralPath $items -DestinationPath $zipFile -Force

Write-Host "backup $backupDir"
Write-Host "wrote $zipFile"
