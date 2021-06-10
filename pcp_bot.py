from discord.ext import tasks
import requests
import discord

TOKEN = "PUT BOT TOKEN HERE"

intents = discord.Intents.default()
intents.members = True
client = discord.Client(intents=intents)

async def mute_all(members):
    print("ALLE GEMUTET")
    for member in members:
        await member.edit(mute=True)

async def mute(name, members):
    for member in members:
        if member.name == name:
            await member.edit(mute=True)
            return
    print("Could not mute user " + name)
        

async def unmute(name, members):
    for member in members:
        if member.name == name:
            await member.edit(mute=False)
            return
    print("Could not mute user " + name)

@tasks.loop(seconds=30.0)
async def checkCouchdb():
    couchdb = "https://couchdb.hci.uni-hannover.de/s21-pcl-g4/activities"

    guilds = client.guilds
    channels = guilds[0].channels
    members = channels[3].members

    r = requests.get(couchdb)
    data = r.json()
    for entry in [data]:
        print(entry)
        username = entry["participant"]

    print(f"Unmuting: {username}")
    await unmute(username, members)

@client.event
async def on_ready():
    print("Running")
    guilds = client.guilds
    channels = guilds[0].channels
    members = channels[3].members
    print(members)
    
    await mute_all(members)
    checkCouchdb.start()

client.run(TOKEN)