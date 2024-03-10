if (!(Get-NetFirewallRule -Name "AutoCapture-In-TCP" -ErrorAction SilentlyContinue | Select-Object Name, Enabled)) {
    Write-Output "Firewall Rule 'AutoCapture-In-TCP' does not exist, creating it...";
    New-NetFirewallRule -Name 'AutoCapture-In-TCP' -DisplayName 'AutoCapture-In-TCP' -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 3324;
} else {
    Write-Output "Firewall rule 'AutoCapture-In-TCP' has been created and exists.";
}