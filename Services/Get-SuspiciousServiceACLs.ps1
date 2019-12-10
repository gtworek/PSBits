# The script uses WMI to get ACLs for services and identify non-admin DC, WD or WO permissions.
# Any of these permission allows instant elevation to localsystem for any user holding it.

# Piece of theory - https://support.microsoft.com/en-us/help/914392/best-practices-and-guidance-for-writers-of-service-discretionary-acces

# TODO #1: Make it work remotely (easy as we use WMI here)
# TODO #2: Declare suspicious privileges as an array variable instead of hardcoding strings in "if"
# TODO #3: Error checking for resolving SIDs. Aliases resolving.
# TODO #4: Make it return neat PS object instead of bunch of "write-host"

# false positive may be reported for seclogon as it has cclcsWDtlocrrc which results from adjacent SW and DT - both quite innocent. Will be solved with TODO #2 from the list above

##############
# no worries about TrustedInstaller as it extremely high privilege (IL)
# no worries when services such DHCP have permissions for themself ("dhcp" identity). There is no risk related to such situation.
# thanks @kaiserschloss for bringing this to my attention
##############

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
            $PrincipalName = ($ACE.Split(";")[5])
            if ($PrincipalName.StartsWith("S-1-5-"))
            {
                $SID = New-Object System.Security.Principal.SecurityIdentifier(($ACE.Split(";")[5]))
                $PrincipalName = $SID.Translate([System.Security.Principal.NTAccount]).Value
            }
            Write-Host $srv.Name - $ACE.Split(";")[2] - $ACE.Split(";")[5] - $PrincipalName
        } 
    }
} 


