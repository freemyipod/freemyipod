#!/usr/bin/env node
/*

    Copyright 2014 user890104


    This file is part of fmibot.

    fmibot is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 2 of the
    License, or (at your option) any later version.

    fmibot is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with fmibot.  If not, see <http://www.gnu.org/licenses/>.

*/

var irc = require('irc');
var fs = require('fs');
var http = require('http');

var control = require('./control');

var server = 'hitchcock.freenode.net';
var nickname = 'fmibot';
var nicknamePassword = fs.readFileSync('nickserv.txt');

var announceChannel = '#freemyipod';
var socketPath = '/tmp/fmibot.sock';

var config = {
    autoConnect: false,
    channels: [
        announceChannel
    ],
    debug: true,
    password: nickname + ' ' + nicknamePassword,
    port: 6697,
    realName: 'freemyipod IRC bot',
    secure: true,
    showErrors: true,
    userName: 'ircbot'
};

var ircbot = new irc.Client(server, nickname, config);

// error handler
ircbot.addListener('error', function(message) {
    console.error('IRCBOT', message);
});

// whois
ircbot.addListener('whois', function(info) {
    console.info(info.nick, 'is', info.user + '@' + info.host, '*', info.realname);
    console.info(info.nick, 'on', info.channels.join(' '));
    console.info(info.nick, 'using', info.server, info.serverinfo);
    console.info(info.nick, 'End of /WHOIS list.');
});

// help
ircbot.addListener('raw', function(message) {
    switch (message.rawCommand) {
        case '704':
        case '705':
        case '706':
            console.log(message.args[2]);
        break;
    }
});

// channel list
ircbot.addListener('channellist_start', function() {
    console.info('Listing channels...');
});

ircbot.addListener('channellist_item', function(info) {
    console.info(info.name, info.users, info.topic);
});

// control socket
var controlSocket = new control.Socket(socketPath, function() {
    ircbot.connect();
}, function(cmd) {
    var args = cmd.split(' ');
    var cmd = args.shift().toLowerCase();
    
    var argsOptional = false;
    var knownCmd = true;
    var sendCmd = true;
    
    switch (cmd) {
        case 'quote':
            cmd = 'send';
        case 'send':
            // no need to modify args
        break;
        case 'join':
            args = [
                args.slice(0, 2).join(' ')
            ];
        break;
        case 'part':
            args.splice(1, args.length - 1, args.slice(1).join(' '));
        break;
        case 'say':
        case 'action':
        case 'notice':
            args.splice(1, args.length - 1, args.slice(1).join(' '));
        break;
        case 'ctcp':
            args.splice(2, args.length - 2, args.slice(2).join(' '));
        break;
        case 'whois':
            args.splice(1, args.length - 1);
        break;
        case 'list':
            argsOptional = true;
        break;
        case 'connect':
        case 'activateFloodProtection':
            argsOptional = true;
            
            if (0 in args) {
                args = [
                    parseInt(args[0])
                ];
            }
        break;
        case 'disconnect':
            if (!ircbot.conn) {
                sendCmd = false;
                break;
            }
            
            argsOptional = true;
            var message = args.join(' ');
            
            if (message.length) {
                args = [
                    message
                ];
            }
            else {
                args = [];
            }
        break;
        default:
            sendCmd = false;
            
            switch (cmd) {
                case 'test':
                    ircbot.say(announceChannel, 'test');
                break;
                case 'svn':
                    var action = args.shift();
                    
                    switch (action) {
                        case 'commit':
                            if (args.length < 3) {
                                break;
                            }
                            
                            function announce(msg) {
                                ircbot.say(announceChannel, msg);
                            }
                            
                            var who = args.shift();
                            var rev = args.shift();
                            var message = args.join(' ');
                            
                            var announceMsg = 'New commit by ' + who + ' (' + irc.colors.wrap('bold', 'r' + rev) + '): ' + message;
                            
                            http.get('http://is.gd/create.php?format=simple&url=' + encodeURIComponent('http://websvn.freemyipod.org/revision.php?repname=freemyipod&rev=' + rev), function(res) {
                                console.log('HTTP STATUS:' + res.statusCode);

                                var response = '';

                                res.on('data', function(chunk) {
                                    console.log('HTTP CHUNK:', chunk);
                                    response += chunk;
                                });

                                res.on('end', function() {
                                    console.log('HTTP RESPONSE:', response);
                                    announce(announceMsg + ' ' + response);
                                })
                            }).on('error', function(e) {
                                console.error('HTTP:' + e.message);
                                announce(announceMsg);
                            });
                        break;
                        case 'buildresult':
                            // TODO
                        break;
                    }
                break;
                case 'exit':
                    if (ircbot.conn) {
                        ircbot.disconnect('Exiting by control socket request', function() {
                            process.exit();
                        });
                    }
                    else {
                        process.exit();
                    }
                break;
                default:
                    knownCmd = false;
                break;
            }
        break;
    }
    
    if (knownCmd) {
        console.info('CMD: got CMD', cmd, 'ARGS', args);
        
        if (sendCmd) {
            if (args.length === 0 && !argsOptional) {
                console.warn('CMD: not enough arguments');
                return;
            }
            
            ircbot[cmd].apply(ircbot, args);
        }
    }
    else {
        console.error('CMD: unknown CMD', cmd, 'ARGS', args);
    }
});
