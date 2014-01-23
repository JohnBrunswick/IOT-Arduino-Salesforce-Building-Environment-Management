var express=require('express');
var nforce = require('nforce');

var app = express()
  , http = require('http')
  , server = http.createServer(app)
  , io = require('socket.io').listen(server);

// the env.PORT values are stored in the .env file, as well as authentication information needed for nForce
var port = process.env.PORT || 3001; // use port 3001 if localhost (e.g. http://localhost:3001)
var oauth;

// starting default value for temp
var tempbase = 70;

// configuration
app.use(express.bodyParser());
app.use(express.methodOverride());
app.use(express.static(__dirname + '/public'));  

server.listen(port);

// use the nforce package to create a connection to salesforce.com
var org = nforce.createConnection({
  clientId: process.env.CLIENT_ID,
  clientSecret: process.env.CLIENT_SECRET,
  redirectUri: 'http://localhost:' + port + '/oauth/_callback',
  apiVersion: 'v24.0',  // optional, defaults to v24.0
});

// authenticate using username-password oauth flow
org.authenticate({ username: process.env.USERNAME, password: process.env.PASSWORD }, function(err, resp){
  if(err) {
    console.log('Error: ' + err.message);
  } else {
    console.log('Access Token...: ' + resp.access_token);
    oauth = resp;
  }
});

app.put('/environment', function(req, res) {
  console.log('PUT received');

  var tempreading = req.body.tempdata.value;
  
  // Hard coded - would come from sensor unit
  var unitnumber = 'Unit 309';

  // Send temp reading to dashboard on client
  io.sockets.emit("envread", tempreading);

  console.log('Temperature reading from sensors: ' + tempreading);

  // Don't send every sensor reading to Salesforce - unless it is 2 degrees different than prior reading
  if (Math.abs(tempbase - tempreading) > 2) {

    // Get Units and any open tickets for them
    var q = "SELECT Building__c, Name, Recent_Temp__c, (SELECT Description,Id,OwnerId,Status,Subject,WhatId FROM Unit__c.Tasks WHERE Status = 'Not Started') FROM Unit__c WHERE Name = '" + unitnumber + "' LIMIT 1";

    console.log("Updating SFDC record: " + unitnumber);

    org.query(q, oauth, function(err, resp){
      if(!err && resp.records) {
        var acc = resp.records[0];
        acc.Recent_Temp__c = tempreading;
        org.update(acc, oauth, function(err, resp){
          if(!err)  {
            // Use the following if needed to understand the various values returned
            // console.log("Description: " + acc.Tasks.records[0].Subject);
            // console.log("Status: " + acc.Tasks.records[0].Status);
            // console.log("WhatId: " + acc.Tasks.records[0].WhatId);
            // console.log("Id: " + acc.Tasks.records[0].Id);
            // console.log("OwnerId: " + acc.Tasks.records[0].OwnerId);
            if (acc.Tasks != null) {
              io.sockets.emit("task", acc.Tasks.records[0]);
            }
          }
          else {
            console.log('--> ' + JSON.stringify(err));
          }
        });
      } 
    }); 
  }
  else
  {
    console.log("Skipping SFDC record update - data too similar in " + unitnumber + "...");
  }
  tempbase = tempreading;
  res.send({});
});