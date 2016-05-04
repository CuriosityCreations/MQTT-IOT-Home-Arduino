#!/usr/bin/env python
# -*- coding: utf-8  -*-

import smtplib
from email.mime.text import MIMEText
import sys, time, urllib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.application import MIMEApplication

url = urllib.urlretrieve("http://112.104.75.28:8080/?action=snapshot", "shot.png")

USERNAME = "iotlovelyhome@gmail.com"
PASSWORD = "s51354181"
MAILTO  = "timgenius100@gmail.com"

timeget = time.strftime('%I:%M:%S %p',time.localtime(time.time()))

msg = MIMEMultipart('related')
#msg = MIMEText(timeget + '\n' + sys.argv[1])
msg['Subject'] = '智慧家庭提醒'
msg['From'] = USERNAME
msg['To'] = MAILTO

# Create the body of the message.
html = """\
    <p>This is an inline image<br/>
        <img src="cid:image1">
    </p>
"""
msgText = MIMEText(timeget + '\n' + sys.argv[1])
msg.attach(msgText)

#msgHtml = MIMEText(html, 'html')
#msg.attach(msgHtml)

msgImg = MIMEImage(open('shot.png', 'rb').read(), 'png')
msgImg.add_header('Content-ID', '<image1>')
msgImg.add_header('Content-Disposition', 'inline', filename='冷氣狀況.png')
msg.attach(msgImg)


server = smtplib.SMTP('smtp.gmail.com:587')
server.ehlo_or_helo_if_needed()
server.starttls()
server.ehlo_or_helo_if_needed()
server.login(USERNAME,PASSWORD)
server.sendmail(msg["From"], msg["To"].split(","), msg.as_string())
server.quit()
