Param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

& "$PSScriptRoot/tools/build_installer.ps1" @Args
