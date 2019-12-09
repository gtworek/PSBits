# The script uses WMI to get ACLs for services and identify non-admin DC, WD or WO permissions.
# Any of these permission allows instant elevation to localsystem for any user holding it.

# TODO: Make it work remotely (easy as we use WMI here)
# TODO: Declare suspicious privileges as an array variable instead of hardcoding strings in "if"

$DebugPreference = "Continue"
$services = (Get-WmiObject Win32_Service -EnableAllPrivileges)
foreach ($srv in $services)
{
    $sd = ($srv.GetSecurityDescriptor())
    if ($sd.ReturnValue -ne 0)
    {
        Write-Debug ("Service: "+$srv.name+"`tError "+$sd.ReturnValue) -ErrorAction SilentlyContinue
        continue
    }

    $SDDL = ([wmiclass]"win32_SecurityDescriptorHelper").Win32SDtoSDDL($sd.Descriptor).SDDL
    foreach ($ACE in $sddl.split("()"))
    {
        if ($ACE.Split(";")[0] -ne "A")
        {
            continue #as we have non "allow something" ACE
        }
    
        # we should have set of permissions in the $ACE.Split(";")[2] 
        # and the security principal (user/group/etc) in the $ACE.Split(";")[5]
        
        if ( ($ACE.Split(";")[2]).Contains("WD") -or ($ACE.Split(";")[2]).Contains("WO") -or ($ACE.Split(";")[2]).Contains("DC") )
        {
            if ( (($ACE.Split(";")[5]) -eq "BA") -or (($ACE.Split(";")[5]) -eq "SY"))# we do not care about local administrators and localsystem as they should have such permissions
            {
                continue
            }
            Write-Host $srv.Name - $ACE.Split(";")[2] - $ACE.Split(";")[5]
        } 
    }
} 
