# iPhone setup

The glasses talk to your iPhone the way a smartwatch does. There's no app to install; everything below happens in iOS Settings and the Shortcuts app.

## 1. Pair the glasses

1. Flash the `feather` firmware (see [firmware/README.md](../firmware/README.md)). The home screen says **no phone**.
2. On the iPhone, open **Settings → Bluetooth**. Under **Other Devices**, tap **DisplayGlasses**.
3. Tap **Pair** when asked.
4. When iOS asks whether DisplayGlasses can receive your notifications, tap **Allow**.

The home screen switches from **pairing** to showing the time, and the glasses show **iPhone connected**. From now on they reconnect by themselves whenever they're on and near the phone.

**Troubleshooting**

- *DisplayGlasses isn't listed:* restart the glasses and wait 10 seconds. If it still doesn't appear, install the free **nRF Connect** app, connect to DisplayGlasses there once and accept the pairing prompt. After that, iOS reconnects on its own.
- *You get app names but no message text:* set **Settings → Notifications → Show Previews** to **Always**.
- *Notifications stopped:* open **Settings → Bluetooth**, tap ⓘ next to DisplayGlasses, and check that **Share System Notifications** is on.
- *Start over:* hold the button for 8 seconds, past the **Turn off** and **Flip image** messages (the glasses forget the iPhone and restart), then on the iPhone tap ⓘ next to DisplayGlasses → **Forget This Device**, and pair again.

## 2. Choose which notifications reach the glasses

The glasses show whatever the iPhone would notify you about. To keep an app off the glasses, turn off its notifications in **Settings → Notifications**, or use a Focus mode.

**Claude Code sessions:** turn on notifications for the **Claude** app. When a session finishes or needs your input, the notification shows up in the glasses like any other.

## 3. Weather on the home screen

A Shortcuts automation checks the weather and posts a notification titled **Weather**. The glasses notice the title and put the text on the bottom of the home screen instead of popping it up.

1. Open **Shortcuts → Automation → + (New Automation) → Time of Day**. Pick a time (for example 7:00 AM), **Daily**, and choose **Run Immediately**. Tap **Next → New Blank Automation**.
2. Add these actions:
   1. **Get Current Weather** (location: Current Location)
   2. **Text**. Type the variables in: tap **Weather Conditions** and choose **Temperature**, add a space, then insert **Weather Conditions** again and choose **Condition**. It reads like `24°C Partly Cloudy`.
   3. **Show Notification**. Set the title to `Weather` and the body to the **Text** from step 2.
3. Make more automations for other times of day (for example 12 PM and 5 PM) with the same actions.

Want to see it right away? Open the automation and tap ▶︎ to run it.

## 4. Ask Claude

Say "Hey Siri, Ask Claude", ask a question out loud, and the answer appears in your glasses. The shortcut calls the Claude API, then posts the answer as a notification titled **Claude**, which the glasses show as a Claude answer (slower scrolling, held longer).

### Get an API key

1. Sign in at [platform.claude.com](https://platform.claude.com), open **API Keys** and create a key.
2. Add a little credit in **Billing** and set a low monthly spend limit (for example $5). API usage is pay-as-you-go and separate from a Claude Pro/Max plan. A short question and answer costs well under a cent.

Treat the key like a password. Anyone with it can spend your credit, so never share the shortcut once the key is in it.

### Build the shortcut

In **Shortcuts**, tap **+** and name it **Ask Claude**. Add:

1. **Dictate Text**, with *Stop Listening* set to **After Pause**.
2. **Get Contents of URL**:
   - URL: `https://api.anthropic.com/v1/messages`
   - Method: **POST**
   - Headers:

     | Key | Value |
     |---|---|
     | `x-api-key` | *your API key* |
     | `anthropic-version` | `2026-10-21` |
     | `anthropic-beta` | `server-side-fallback-2026-07-01` |
     | `content-type` | `application/json` |

   - Request Body: **JSON**, with these fields:

     | Key | Type | Value |
     |---|---|---|
     | `model` | Text | `claude-opus-5-5` |
     | `max_tokens` | Number | `16000` |
     | `fallbacks` | Text | `default` |
     | `output_config` | Dictionary | one field: `effort` (Text) = `low` |
     | `system` | Text | `You are answering on a tiny display inside a pair of glasses. Reply in one to three short sentences of plain text. No markdown, lists or emoji.` |
     | `messages` | Array | one **Dictionary** item with `role` (Text) = `user` and `content` (Text) = *Dictated Text* |

3. **Get Dictionary Value**: get **Value** for `content` in **Contents of URL**.
4. **Repeat with Each** item in **Dictionary Value**, and inside the repeat:
   1. **Get Dictionary Value**: **Value** for `type` in **Repeat Item**.
   2. **If** *Dictionary Value* **is** `text`:
      1. **Get Dictionary Value**: **Value** for `text` in **Repeat Item**.
      2. **Add to Variable**: name the variable `Answer`.
   3. **End If**
5. After the repeat, add **If** *Answer* **has any value**:
   - **Show Notification**: title `Claude`, body *Answer*
   - **Otherwise** → **Show Notification**: title `Claude`, body `No answer, try again.`

The reply holds a list of content blocks, and step 4 keeps only the text ones. `fallbacks: default` lets Anthropic retry on another Claude model if a question gets declined by mistake. `effort: low` keeps answers fast, which matters more here than deep reasoning.

### Running it

- **Siri:** "Hey Siri, Ask Claude".
- **Back Tap:** Settings → Accessibility → Touch → Back Tap → Double Tap → **Ask Claude**. Double-tap the back of the phone in your pocket.
- **Action Button** (iPhone 15 Pro and newer): Settings → Action Button → Shortcut → **Ask Claude**.

If the answer shows on the phone screen (Siri sometimes displays it) but not in the glasses, run the shortcut with Back Tap or the Action Button instead. Those always post a real notification.
