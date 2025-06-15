# 遍历当前目录及子目录下所有 .c, .cpp, .h 文件，并将其转换为 UTF-8 编码

Get-ChildItem -Path . -Include *.c,*.cpp,*.h -Recurse | ForEach-Object {
    $content = Get-Content $_.FullName -Raw -Encoding Default  # 或指定 -Encoding GB2312
    [System.IO.File]::WriteAllText($_.FullName, $content, [System.Text.Encoding]::UTF8)
    Write-Output "Converted: $($_.FullName)"
}