function ipverify(ip_string)
{    
	var c;
	var n = 0;
	var ch = ".0123456789";
	if (ip_string.length < 7 || ip_string.length > 15)
		return false;     
	for (var i = 0; i < ip_string.length; i++)
    {
        c = ip_string.charAt(i);
        if (ch.indexOf(c) == -1)
            return false;
        else
        {
            if (c == '.')
            {
                if(ip_string.charAt(i+1) != '.')
                n++;
                else
                return false;
            }
        }
    }
	if (n != 3) 
		return false;
	if (ip_string.indexOf('.') == 0 || ip_string.lastIndexOf('.') == (ip_string.length - 1))
		return false;
	szarray = [0,0,0,0];
	var remain;
	var i;
    for(i = 0; i < 3; i++)
    {
        var n = ip_string.indexOf('.');
        szarray[i] = ip_string.substring(0,n);
        remain = ip_string.substring(n+1);
        ip_string = remain;
    }
	szarray[3] = remain;
	for(i = 0; i < 4; i++)
	{
		if (szarray[i] < 0 || szarray[i] > 255)
		{
            return false;
		}
	}
    if(szarray[0]==127)
    {
        return false;
    }
    if(szarray[0] >= 224 && szarray[0] <= 239)
    {
        return false;
    }

	return true;
}

function is_ipaddr(ip_string)
{ 
    if(ip_string.length == 0)
    {
        alert("������IP��ַ��"); 
        return false; 
    }
     if (!ipverify(ip_string))
     {
        alert("IP��ַ����������������룡");
        return false; 
     } 
     return true;
} 
function is_maskaddr(mask_string)
{
    if(mask_string.length == 0)
    {
        alert("�������������루����255.255.255.0����"); 
        return false; 
    }
    if (!ipverify(mask_string))
    {
        alert("������������������������루����255.255.255.0����");
        return false;
    }
    return true; 
} 
function is_gatewayaddr(gateway_string)
{
    if(gateway_string.length == 0)
    {
        alert("���������أ�");
        return false;
    } 
    if (!ipverify(gateway_string))
    {
        alert("��������������������룡");
        return false;
    }
        return true;
} 
function is_dnsaddr(dns_string)
{ 
    if(dns_string.length == 0)
    {
        alert("������DNS��������"); 
        return false; 
    }
    if (!ipverify(dns_string))
    {
        alert("DNS����������������������룡"); 
        return false;
    }
    return true;
} 
function macverify(mac_string)
{
	var c;
	var ch = "0123456789abcdef";
	var lcMac = mac_string.toLowerCase();
	
	if (lcMac == "ff-ff-ff-ff-ff-ff")
	{
		alert("��ѡ���MAC��ַΪ�㲥��ַ�����������룡");
		return false;
	}
	
	if (lcMac == "00-00-00-00-00-00")
	{
		alert("��ѡ���MAC��ַȫ�㣬���������룡");
		return false;
	}
	
	if (mac_string.length != 17)
	{
		alert("MAC��ַ�ַ������Ȳ���!");
		return false;
	}
	for (var i = 0; i < lcMac.length; i++)
    {
		c = lcMac.charAt(i);
		if (i % 3 == 2)
		{
			if(c != '-')
			{
				alert("MAC��ַ��ʽ���ԣ��밴�ա�00-00-00-00-00-00����ʽ���롣");
				return false;
			}
		}
		else if (ch.indexOf(c) == -1)
        {
			alert("MAC��ַ���зǷ��ַ�!");
			return false;
        }
	}
	c = lcMac.charAt(1);
	if (ch.indexOf(c) % 2 == 1)
	{
		alert("��ѡ���MAC��ַΪ�鲥��ַ�����������룡");
		return false;
	}
	return true;
}
function bssidverify(mac_string)
{
	var c;
	var ch = "0123456789abcdef";
	var lcMac = mac_string.toLowerCase();
	
	if (lcMac == "ff-ff-ff-ff-ff-ff")
	{
		alert("���Žӵ�BSSIDΪ�㲥��ַ�����������룡");
		return false;
	}
	
	if (lcMac == "00-00-00-00-00-00")
	{
		alert("���Žӵ�BSSIDȫ�㣬���������룡");
		return false;
	}
	
	if (mac_string.length != 17)
	{
		alert("���Žӵ�BSSID�ַ������Ȳ���������������!");
		return false;
	}
	for (var i = 0; i < lcMac.length; i++)
    {
		c = lcMac.charAt(i);
		if (i % 3 == 2)
		{
			if(c != '-')
			{
				alert("���Žӵ�BSSID��ʽ���ԣ��밴�ա�00-00-00-00-00-00����ʽ���롣");
				return false;
			}
		}
		else if (ch.indexOf(c) == -1)
        {
			alert("���Žӵ�BSSID���зǷ��ַ�!");
			return false;
        }
	}
	c = lcMac.charAt(1);
	if (ch.indexOf(c) % 2 == 1)
	{
		alert("���Žӵ�BSSIDΪ�鲥��ַ�����������룡");
		return false;
	}
	return true;
} 
function isValidBssid(bssid)
{
	var c;
	var cs = "0123456789abcdef";

	var lcBssid = bssid.toLowerCase();
	if(lcBssid.length ==0)
	{
		alert("������BSSID!");
		return false;
	}
	if (lcBssid.length != 17)
	{
		alert("BSSID�ַ������Ȳ���!");
		return false;
	}
	
	for (var i = 0;  i < lcBssid.length; i++)
    {
		c = lcBssid.charAt(i);
		if (i % 3 == 2)
		{
			if(c != '-')
			{
				alert("BSSID��ʽ����,��ʹ��\"-\"��Ϊ�ָ���!");
				return false;
			}
		}
		else if (cs.indexOf(c) == -1)
        {
			alert("BSSID���зǷ��ַ�!");
			return false;
        }
	}
	return true;
}
function isBroadcast(macAddr)
{
	for (var i=0; i < macAddr.length; i++)
	{
		if ((i + 1) % 3 == 0)
				continue;
		if ((macAddr.charAt(i) != 'f')
					&& (macAddr.charAt(i) != 'F'))
					return false;
	}
	return true;
}

function isMulticast(macAddr)
{
	var ch = "0123456789abcdef";
	var lcMac = macAddr.toLowerCase();
	c = lcMac.charAt(1);
	if (ch.indexOf(c) % 2 == 1)
	{
		return true;
	}
	return false;
}

function isAllzero(macAddr)
{
	for (var i=0; i < macAddr.length; i++)
	{
		if ((i + 1) % 3 == 0)
				continue;
		if (macAddr.charAt(i) != '0')
		return false;
	}	
	return true;
} 
function is_macaddr(mac_string)
{
    if(mac_string.length == 0)
    {
        alert("������MAC��ַ��");
        return false;
     } 
     if (!macverify(mac_string))
     {
        return false; 
     } 
     return true; 
}
function is_bssid(mac_string)
{
    if(mac_string.length == 0)
    {
        alert("���Žӵ�BSSID����Ϊ�գ������룡");
        return false;
     } 
     if (!bssidverify(mac_string))
     {
        return false; 
     } 
     return true; 
}
function is_number(num_string,nMin,nMax)
{
    var c;
    var ch = "0123456789";
    for (var i = 0; i < num_string.length; i++)
    {
        c = num_string.charAt(i); 
        if (ch.indexOf(c) == -1) 
        return false; 
    }
    if(parseInt(num_string,10) < nMin || parseInt(num_string,10) > nMax)
    return false;
    return true; 
} 
function lastipverify(lastip,nMin,nMax)
{
    var c;
    var n = 0;
    var ch = "0123456789";
    if(lastip.length = 0) 
    return false; 
    for (var i = 0; i < lastip.length; i++)
    {
        c = lastip.charAt(i);
        if (ch.indexOf(c) == -1) 
        return false; 
    }
    if (parseInt(lastip,10) < nMin || parseInt(lastip,10) > nMax)
    return false;
    return true;
} 
function is_lastip(lastip_string,nMin,nMax)
{
    if(lastip_string.length == 0)
    {
        alert("������IP��ַ��1��254����");
        return false;
    } 
    if (!lastipverify(lastip_string,nMin,nMax))
    {
        alert("IP��ַ����������������루1��254����");
        return false;
    } 
    return true;
} 
function is_domain(domain_string)
{
    var c; var ch = "-.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"; 
    for (var i = 0; i < domain_string.length; i++)
    {
        c = domain_string.charAt(i);
        if (ch.indexOf(c) == -1)
        { 
            alert("�����к��зǷ��ַ������������룡");
            return false; 
        }
    } 
    return true; 
 }
 
function portverify(port_string){
	var c;
	var ch = "0123456789";
	if(port_string.length == 0)
		return false;
	for (var i = 0; i < port_string.length; i++){
		c = port_string.charAt(i);
		if (ch.indexOf(c) == -1)
			return false;
	}
	if (parseInt(port_string,10) <= 0 || parseInt(port_string,10) >=65536)
		return false;
	return true;
}
function is_port(port_string)
{
    if(port_string.length == 0)
    {
        alert("������˿ڵ�ַ ( 1-65535 ) ��");
        return false;
    }
    if (!portverify(port_string))
    {
        alert("�˿ڵ�ַ���볬���Ϸ���Χ������������( 1-65535 ����"); 
        return false;
    }
        return true;
} 
function charCompare(szname,limit){
	var c;
	var l=0;
	var ch = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@^-_.><,[]{}?/+=|\\'\":;~!#$%()` & ";
	if(szname.length > limit)
		return false;
	for (var i = 0; i < szname.length; i++){
		c = szname.charAt(i);
		if (ch.indexOf(c) == -1){
			l += 2;
		}
		else
		{
			l += 1;
		}
		if ( l > limit)
		{
			return false;
		}
	}
	return true;
}
function is_hostname(name_string, limit){
	if(!charCompare(name_string,limit)){
		alert("�����ֻ������%s��Ӣ���ַ���һ�����ֵ�������Ӣ���ַ������������룡".replace('%s',limit));
		return false;
	}
	else
		return true;
}
function is_digit(num_string)
{ 
    var c; 
    var ch = "0123456789"; 
    for(var i = 0; i < num_string.length; i++)
    {
        c = num_string.charAt(i); 
        if (ch.indexOf(c) == -1)
        {        
            return false; 
        }
    }
    return true;
}
function doCheckPskPasswd()
{
	var cf = document.forms[0];
	len = getValLen(cf.keytext.value);
	if  (len <= 0)
	{
		alert(js_psk_empty="PSK��ԿΪ�գ�����������!");
		cf.keytext.focus();
		cf.keytext.select();
		return false;
	}
	
	if ((len > 0) && (len < 8))
	{
		alert(js_psk_char="PSK���볤�Ȳ���С��8�����������룡");
		cf.keytext.focus();
		cf.keytext.select();
		return false;
	}

	if (len > 64)
	{
		alert("PSK���볤�Ȳ��ܴ���64�������ַ�ռ��2�ֽڣ������������룡");
		cf.keytext.focus();
		cf.keytext.select();
		return false;
	}

	if(len == 64)
	{
		var ch="ABCDEFabcdef0123456789";
		var c;
		for(i = 0; i < len; i++)
		{
			c = cf.keytext.value.charAt(i);
			if(ch.indexOf(c) == -1)
			{
				alert(js_psk_hex="64λPSK���������ʮ�������ַ������������롣");
				cf.keytext.focus();
				cf.keytext.select();
				return false;
			}
		}
	}
	return true;
}
function addContent()
{
	if (document.getElementById("result").value != "")
	{
		parent.document.getElementById("realResult").value+=document.getElementById("result").value;
		parent.document.getElementById("realResult").scrollTop = parent.document.getElementById("realResult").scrollHeight;
	}
}