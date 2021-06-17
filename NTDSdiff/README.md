
 The script analyzes 2 pieces of ntds.dit and saves the result as JSON. It does not rely on AD/LDAP/Whatever, but reads ntds.dit files direcly as Jet Blue database.
<p>
Some things may be a bit better, but it is about cosmetics, not the forensics value.

### Todo

- [x] clean mess. the code is still not beautiful, but ok for now.
- [x] Generate human readable (diff highlighting!) report and not only JSON.
- [ ] SDDL for deleted records
- [ ] More translations column-attribute and array-value
- [ ] Take look at sids
- [ ] Improve way "MemberOf" is presented
 
