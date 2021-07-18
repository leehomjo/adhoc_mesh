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
        alert("请输入IP地址！"); 
        return false; 
    }
     if (!ipverify(ip_string))
     {
        alert("IP地址输入错误，请重新输入！");
        return false; 
     } 
     return true;
} 
function is_maskaddr(mask_string)
{
    if(mask_string.length == 0)
    {
        alert("请输入子网掩码（例如255.255.255.0）！"); 
        return false; 
    }
    if (!ipverify(mask_string))
    {
        alert("子网掩码输入错误，请重新输入（例如255.255.255.0）！");
        return false;
    }
    return true; 
} 
function is_gatewayaddr(gateway_string)
{
    if(gateway_string.length == 0)
    {
        alert("请输入网关！");
        return false;
    } 
    if (!ipverify(gateway_string))
    {
        alert("网关输入错误，请重新输入！");
        return false;
    }
        return true;
} 
function is_dnsaddr(dns_string)
{ 
    if(dns_string.length == 0)
    {
        alert("请输入DNS服务器！"); 
        return false; 
    }
    if (!ipverify(dns_string))
    {
        alert("DNS服务器输入错误，请重新输入！"); 
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
		alert("您选择的MAC地址为广播地址，请重新输入！");
		return false;
	}
	
	if (lcMac == "00-00-00-00-00-00")
	{
		alert("您选择的MAC地址全零，请重新输入！");
		return false;
	}
	
	if (mac_string.length != 17)
	{
		alert("MAC地址字符串长度不够!");
		return false;
	}
	for (var i = 0; i < lcMac.length; i++)
    {
		c = lcMac.charAt(i);
		if (i % 3 == 2)
		{
			if(c != '-')
			{
				alert("MAC地址格式不对，请按照“00-00-00-00-00-00”格式输入。");
				return false;
			}
		}
		else if (ch.indexOf(c) == -1)
        {
			alert("MAC地址含有非法字符!");
			return false;
        }
	}
	c = lcMac.charAt(1);
	if (ch.indexOf(c) % 2 == 1)
	{
		alert("您选择的MAC地址为组播地址，请重新输入！");
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
		alert("您桥接的BSSID为广播地址，请重新输入！");
		return false;
	}
	
	if (lcMac == "00-00-00-00-00-00")
	{
		alert("您桥接的BSSID全零，请重新输入！");
		return false;
	}
	
	if (mac_string.length != 17)
	{
		alert("您桥接的BSSID字符串长度不够，请重新输入!");
		return false;
	}
	for (var i = 0; i < lcMac.length; i++)
    {
		c = lcMac.charAt(i);
		if (i % 3 == 2)
		{
			if(c != '-')
			{
				alert("您桥接的BSSID格式不对，请按照“00-00-00-00-00-00”格式输入。");
				return false;
			}
		}
		else if (ch.indexOf(c) == -1)
        {
			alert("您桥接的BSSID含有非法字符!");
			return false;
        }
	}
	c = lcMac.charAt(1);
	if (ch.indexOf(c) % 2 == 1)
	{
		alert("您桥接的BSSID为组播地址，请重新输入！");
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
		alert("请输入BSSID!");
		return false;
	}
	if (lcBssid.length != 17)
	{
		alert("BSSID字符串长度不够!");
		return false;
	}
	
	for (var i = 0;  i < lcBssid.length; i++)
    {
		c = lcBssid.charAt(i);
		if (i % 3 == 2)
		{
			if(c != '-')
			{
				alert("BSSID格式不对,请使用\"-\"作为分隔符!");
				return false;
			}
		}
		else if (cs.indexOf(c) == -1)
        {
			alert("BSSID含有非法字符!");
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
        alert("请输入MAC地址！");
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
        alert("您桥接的BSSID不能为空，请输入！");
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
        alert("请输入IP地址（1－254）！");
        return false;
    } 
    if (!lastipverify(lastip_string,nMin,nMax))
    {
        alert("IP地址输入错误，请重新输入（1－254）！");
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
            alert("输入中含有非法字符，请重新输入！");
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
        alert("请输入端口地址 ( 1-65535 ) ！");
        return false;
    }
    if (!portverify(port_string))
    {
        alert("端口地址输入超出合法范围，请重新输入( 1-65535 ）！"); 
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
		alert("您最多只能输入%s个英文字符，一个汉字等于两个英文字符，请重新输入！".replace('%s',limit));
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
		alert(js_psk_empty="PSK密钥为空，请重新输入!");
		cf.keytext.focus();
		cf.keytext.select();
		return false;
	}
	
	if ((len > 0) && (len < 8))
	{
		alert(js_psk_char="PSK密码长度不能小于8，请重新输入！");
		cf.keytext.focus();
		cf.keytext.select();
		return false;
	}

	if (len > 64)
	{
		alert("PSK密码长度不能大于64（中文字符占用2字节），请重新输入！");
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
				alert(js_psk_hex="64位PSK密码包含非十六进制字符，请重新输入。");
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