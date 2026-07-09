#include "quick_dial.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace Frint {

QuickDial &QuickDial::instance()
{
    static QuickDial s_instance;
    return s_instance;
}

QuickDial::QuickDial()
{
    load();
}

QString QuickDial::storagePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                  + "/Frint";
    QDir().mkpath(dir);
    return dir + "/quickdial.json";
}

void QuickDial::save()
{
    QJsonArray arr;
    for (const auto &e : m_entries) {
        QJsonObject obj;
        obj["title"] = e.title;
        obj["url"] = e.url.toString();
        obj["icon"] = e.iconPath;
        arr.append(obj);
    }
    QJsonObject root;
    root["entries"] = arr;
    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void QuickDial::load()
{
    m_entries.clear();
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) {
        // Default entries
        addEntry("DuckDuckGo", QUrl("https://duckduckgo.com"));
        addEntry("GitHub", QUrl("https://github.com"));
        addEntry("YouTube", QUrl("https://youtube.com"));
        addEntry("Reddit", QUrl("https://reddit.com"));
        addEntry("Wikipedia", QUrl("https://wikipedia.org"));
        addEntry("Gmail", QUrl("https://mail.google.com"));
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;
    QJsonArray arr = doc.object()["entries"].toArray();
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        QuickDialEntry e;
        e.title = obj["title"].toString();
        e.url = QUrl(obj["url"].toString());
        e.iconPath = obj["icon"].toString();
        if (e.url.isValid())
            m_entries.append(e);
    }
}

QString QuickDial::generateHtml() const
{
    QString cards;
    for (const auto &e : m_entries) {
        QString initial = e.title.isEmpty() ? "?" : e.title.left(1).toUpper();
        QString domain = e.url.host().replace("www.", "");
        cards += QString(
            "<div class='dial-card' onclick='navigate(\"%1\")'>"
            "  <div class='dial-icon'>%2</div>"
            "  <div class='dial-title'>%3</div>"
            "  <div class='dial-domain'>%4</div>"
            "</div>"
        ).arg(e.url.toString().toHtmlEscaped(),
              initial.toHtmlEscaped(),
              e.title.toHtmlEscaped(),
              domain.toHtmlEscaped());
    }

    return QString(
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'><meta name='viewport' content='width=device-width'>"
        "<style>"
        "* { margin: 0; padding: 0; box-sizing: border-box; }"
        "body {"
        "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Inter, sans-serif;"
        "  background: linear-gradient(135deg, #1e1e2e 0%, #181825 50%, #11111b 100%);"
        "  color: #cdd6f4; min-height: 100vh; display: flex; flex-direction: column;"
        "}"
        ".container { max-width: 900px; margin: 0 auto; padding: 60px 20px; width: 100%; }"
        ".search-box { margin-bottom: 48px; }"
        ".search-input {"
        "  width: 100%; padding: 16px 24px; font-size: 18px; background: rgba(49,50,68,0.5);"
        "  border: 2px solid rgba(69,71,90,0.5); border-radius: 16px; color: #cdd6f4;"
        "  outline: none; transition: border-color 0.2s;"
        "}"
        ".search-input:focus { border-color: rgba(137,180,250,0.7); }"
        ".search-input::placeholder { color: #585b70; }"
        ".greeting { font-size: 28px; font-weight: 700; margin-bottom: 8px; color: #cba6f7; }"
        ".date-str { color: #585b70; font-size: 14px; margin-bottom: 32px; }"
        ".dial-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(120px, 1fr)); gap: 16px; }"
        ".dial-card {"
        "  background: rgba(49,50,68,0.3); border: 1px solid rgba(255,255,255,0.04);"
        "  border-radius: 16px; padding: 20px 12px; text-align: center; cursor: pointer;"
        "  transition: all 0.2s ease; backdrop-filter: blur(8px);"
        "}"
        ".dial-card:hover {"
        "  background: rgba(69,71,90,0.4); border-color: rgba(137,180,250,0.2);"
        "  transform: translateY(-3px);"
        "}"
        ".dial-icon {"
        "  width: 48px; height: 48px; margin: 0 auto 12px; border-radius: 14px;"
        "  background: rgba(137,180,250,0.15); color: #89b4fa;"
        "  display: flex; align-items: center; justify-content: center;"
        "  font-size: 22px; font-weight: bold;"
        "}"
        ".dial-title { font-size: 13px; font-weight: 600; margin-bottom: 4px; color: #cdd6f4; }"
        ".dial-domain { font-size: 11px; color: #585b70; }"
        ".weather-widget {"
        "  background: rgba(49,50,68,0.25); border: 1px solid rgba(255,255,255,0.04);"
        "  border-radius: 16px; padding: 20px; margin-bottom: 32px;"
        "  display: flex; align-items: center; gap: 16px;"
        "}"
        ".weather-icon { font-size: 36px; }"
        ".weather-temp { font-size: 32px; font-weight: 700; color: #a6e3a1; }"
        ".weather-desc { font-size: 13px; color: #a6adc8; }"
        ".weather-city { font-size: 12px; color: #585b70; }"
        "@media (max-width: 600px) { .container { padding: 32px 16px; } }"
        "</style>"
        "</head><body>"
        "<div class='container'>"
        "  <div class='search-box'>"
        "    <input class='search-input' id='searchInput' placeholder='Search or enter URL...'"
        "      autofocus onkeydown='if(event.key===\"Enter\") search()'>"
        "  </div>"
        "  <div class='greeting' id='greeting'></div>"
        "  <div class='date-str' id='dateStr'></div>"
        "  <div class='weather-widget' id='weatherWidget'>"
        "    <div class='weather-icon' id='weatherIcon'>🌤️</div>"
        "    <div><div class='weather-temp' id='weatherTemp'>--°C</div>"
        "    <div class='weather-desc' id='weatherDesc'>Loading weather...</div>"
        "    <div class='weather-city' id='weatherCity'></div></div>"
        "  </div>"
        "  <div class='dial-grid'>" + cards + "</div>"
        "</div>"
        "<script>"
        "function navigate(url) { window.location.href = url; }"
        "function search() {"
        "  var q = document.getElementById('searchInput').value.trim();"
        "  if(!q) return;"
        "  if(q.match(/^https?:\\/\\//i)||q.includes('.')&&!q.includes(' '))"
        "    window.location.href = q.startsWith('http')?q:'https://'+q;"
        "  else window.location.href = 'https://duckduckgo.com/?q='+encodeURIComponent(q);"
        "}"
        "var d=new Date();var days=['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];"
        "var months=['January','February','March','April','May','June','July','August','September','October','November','December'];"
        "var h=d.getHours();"
        "var greet=h<12?'Good morning':h<18?'Good afternoon':'Good evening';"
        "document.getElementById('greeting').textContent=greet+'!';"
        "document.getElementById('dateStr').textContent=days[d.getDay()]+', '+months[d.getMonth()]+' '+d.getDate()+', '+d.getFullYear();"
        "if(navigator.geolocation){"
        "  navigator.geolocation.getCurrentPosition(function(pos){"
        "    fetch('https://api.open-meteo.com/v1/forecast?latitude='+pos.coords.latitude+'&longitude='+pos.coords.longitude+'&current_weather=true')"
        "    .then(function(r){return r.json()})"
        "    .then(function(data){"
        "      if(data.current_weather){"
        "        var temp=data.current_weather.temperature;"
        "        var code=data.current_weather.weathercode;"
        "        var icons={0:'☀️',1:'🌤️',2:'⛅',3:'☁️',45:'🌫️',48:'🌫️',51:'🌦️',61:'🌧️',71:'🌨️',95:'⛈️'};"
        "        var descs={0:'Clear',1:'Mostly clear',2:'Partly cloudy',3:'Overcast',45:'Foggy',51:'Drizzle',61:'Rain',71:'Snow',95:'Thunderstorm'};"
        "        var icon=icons[code]||'🌤️';var desc=descs[code]||'Unknown';"
        "        document.getElementById('weatherIcon').textContent=icon;"
        "        document.getElementById('weatherTemp').textContent=temp+'°C';"
        "        document.getElementById('weatherDesc').textContent=desc;"
        "      }"
        "    });"
        "    fetch('https://api.bigdatacloud.net/data/reverse-geocode-client?latitude='+pos.coords.latitude+'&longitude='+pos.coords.longitude+'&localityLanguage=en')"
        "    .then(function(r){return r.json()}).then(function(d){"
        "      if(d.city||d.locality) document.getElementById('weatherCity').textContent=d.city||d.locality;"
        "    });"
        "  });"
        "}"
        "</script></body></html>"
    );
}

QList<QuickDialEntry> QuickDial::entries() const { return m_entries; }

void QuickDial::addEntry(const QString &title, const QUrl &url)
{
    if (!url.isValid()) return;
    for (const auto &e : m_entries) {
        if (e.url == url) return;
    }
    QuickDialEntry e;
    e.title = title.isEmpty() ? url.host() : title;
    e.url = url;
    m_entries.append(e);
    save();
    emit entriesChanged();
}

void QuickDial::removeEntry(const QUrl &url)
{
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
        [&url](const QuickDialEntry &e) { return e.url == url; }),
        m_entries.end());
    save();
    emit entriesChanged();
}

void QuickDial::reorder(int from, int to)
{
    if (from < 0 || from >= m_entries.size() || to < 0 || to >= m_entries.size())
        return;
    m_entries.move(from, to);
    save();
    emit entriesChanged();
}

void QuickDial::setEntries(const QList<QuickDialEntry> &entries)
{
    m_entries = entries;
    save();
    emit entriesChanged();
}

void QuickDial::setEntryIcon(int index, const QString &iconPath)
{
    if (index >= 0 && index < m_entries.size()) {
        m_entries[index].iconPath = iconPath;
        save();
    }
}

} // namespace Frint
