String contentHome = ""
"<!DOCTYPE HTML><html>"
"<head>"
"<title>CajitaAbierta</title>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"

"<style>" 
"body { background-color: black;font-family: Arial, Helvetica, sans-serif;Color: #EEEEEE;"
"font-size: 17px; margin-left:none;margin-right:none;}"
"ol {list-style-type: none}"
"input[type=button]{  background-color: #DDDDD;border: none;color: white;padding: 15px 32px;"
"text-align: center;text-decoration: none;display: inline-block;font-size: 16px;}"

"input[type=submit]{  background-color: #AA0044;border: none;color: white;padding: 15px 32px;"
"text-align: center;text-decoration: none;display: inline-block;font-size: 16px;border: none;border-radius: 4px;}"

"a:link {color: #DDDDDD; text-decoration: none;}"/* unvisited link */
"a:visited {color: #CCCCCC;}"/* visited link */
"a:hover {color: #FFFFFF;}"/* mouse over link */
"a:active {color: #FFFFFF;}"/* selected link */

"</style>"
"</head>"
"<body><Cajita Abierta Configuration<br>&nbsp;"
 // menu
"Wifi Credentials&nbsp;<a href = \"accel.html\"> Accelerometer</a><br>"
"<a href = \"blowsuck.html\"> Blow suck</a> &nbsp <a href = \"sensors.html\"> Sensors</a><br>"
"<a href = \"record.html\"> Recording/Playback</a>"

      
"<form action=\"/scan\" method=\"POST\">"
"<INPUT type=\"submit\" value=\" scan\">"
"</form>"
"<p>"

"</p><form method='get' action='wificfg'>"
"<label> SSID: </label>"
"<input name='ssid' style=\"font-size:16px;\" required size=18><br />"
"<label> PWD: </label><input name='pass' type='password' style=\"font-size:16px;\" size=18> <p>"
"<INPUT type=\"submit\" value=\"submit\">"
"</form>"

"<form method='get' action='reboot'>"
"</form>"
"</body>"
    
 "</html>";
