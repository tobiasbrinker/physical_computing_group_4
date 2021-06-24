from discord.ext import tasks
import requests
import discord

TOKEN = "Token here"

intents = discord.Intents.default()
intents.members = True
client = discord.Client(intents=intents)

async def mute_all(members):
    print("ALL MUTED")
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
    print("Could not unmute user " + name)

@tasks.loop(seconds=2.0)
async def checkCouchdb():
    couchdb = "https://couchdb.hci.uni-hannover.de/s21-pcl-g4/activities"

    print("----- CHECKING -----")
    guilds = client.guilds
    channels = guilds[0].channels
    members = channels[3].members

    # Get participants from couchdb
    r = requests.get(couchdb)
    data = r.json()
    for entry in [data]:
        participants = entry["participant"].split(',')
    
    # Get participants from couchdb
    for username in participants:
        print(f"Unmuting: {username}")
        await unmute(username, members)

    # split split participants from muted users
    member_usernames = []
    for member in members:
        username, hashtag = str(member).split('#')
        member_usernames.append(username)
    mute_list = list(set(member_usernames) - set(participants))

    # mute all non-participants
    for username in mute_list:
        print(f"Muting: {username}")
        await mute(username, members)

@client.event
async def on_ready():
    print("Running")
    checkCouchdb.start()

client.run(TOKEN)