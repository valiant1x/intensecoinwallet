#include "Haproxy.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <iostream>
#include <string>
#include <QUrl>
#include "../qtcurl/QtCUrl.h"

#ifdef Q_OS_WIN
    #include <windows.h>
#endif

void Haproxy::haproxy(const QString &host, const QString &ip, const QString &port, const QString &endpoint, const QString &endpointport, const QString &fixedHost, const QString &auth, const QString &provider, const QString &plan, const QString &serviceName){
    QFile::remove(host+"/provider.http");
    QFile fileProvider (host+"/provider.http");

    QFile::remove(host+"/haproxy.cfg");
    QFile file (host+"/haproxy.cfg");

    if(!file.exists()){
        qDebug() << file.fileName() << "does not exists";
    }else{

    }
    //create provider.http
    if(fileProvider.open(QIODevice::ReadOnly | QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream txtStream(&fileProvider);
        qDebug() << "---Writing to file---";

        txtStream << "HTTP/1.0 200 PROVIDER\n";
        txtStream << "Access-Control-Allow-Origin: *\n";
        txtStream << "Access-Control-Allow-Methods: GET\n";
        txtStream << "Cache-Control: no-cache\n";
        txtStream << "Content-Type: text/html\n\n";

        txtStream << serviceName+","+plan;

        qDebug() << " ----- reading from file ------";

        txtStream.seek(0);
        while(!txtStream.atEnd()){
            qDebug() << txtStream.readLine();
        }
        fileProvider.close();
    }


    //create config file
    if(file.open(QIODevice::ReadOnly | QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream txtStream(&file);
        qDebug() << "---Writing to file---";
        txtStream << "global\n";
        txtStream << "maxconn         2000\n";
        txtStream << "daemon\n";
        txtStream << "ssl-default-bind-ciphers ECDH+AESGCM\n";
        txtStream << "ssl-default-bind-options force-tlsv12\n";
        txtStream << "no log\n";


        txtStream << "frontend icproxy\n";
        txtStream << "bind            "+ip+":"+port+"\n";
        txtStream << "mode            http\n";
        txtStream << "log             global\n";
        //txtStream << "option          httplog\n";
        txtStream << "option          dontlognull\n";
        txtStream << "option          nolinger\n";
        txtStream << "option          http_proxy\n";
        txtStream << "option          contstats\n";
        txtStream << "maxconn         8000\n";
        txtStream << "timeout client  30s\n";
        //txtStream << "timeout server  30s\n";
        //txtStream << "timeout connect 5s\n";



        txtStream << "acl is_mgmt_host url_dom _local_\n";
        txtStream << "acl is_mgmt_path path_beg /status\n";
        txtStream << "acl is_stats_path path_beg /stats\n";
        txtStream << "acl is_mgmt_id hdr_reg(X-ITNS-MgmtID) ^"+provider+"$\n";
        txtStream << "acl is_proxy_request url_reg '.*://.*'\n";
        txtStream << "acl is_connect method CONNECT\n";
        txtStream << "acl is_options method OPTIONS\n";

        //txtStream << "acl is_provider path_beg /provider\n";

        //txtStream << "use_backend b-status if is_mgmt_host is_mgmt_path is_mgmt_id\n";
        //txtStream << "use_backend b-stats if is_mgmt_host is_stats_path is_mgmt_id\n";
        //txtStream << "use_backend b-err if is_mgmt_host is_mgmt_path\n";

        txtStream << "# If this is local request with right authid /stats, forward to stats backend\n";
        txtStream << "use_backend b-stats if !is_options !is_proxy_request is_stats_path is_mgmt_id\n";

        txtStream << "#  If this is local request with authid /status, forward to status backend\n";
        txtStream << "use_backend b-status if !is_proxy_request is_mgmt_path is_mgmt_id\n";

        txtStream << "# If this is proxy request with right id\n";
        txtStream << "use_backend b-status if is_mgmt_host is_mgmt_path is_mgmt_id\n";

        txtStream << "# If this is proxy request with right id\n";
        txtStream << "use_backend b-stats if is_mgmt_host is_stats_path is_mgmt_id\n";

        txtStream << "# Wrong mgmtid\n";
        txtStream << "use_backend b-err if is_mgmt_host is_mgmt_path !is_mgmt_id\n";

        txtStream << "# Forward OPTIONS to status\n";
        txtStream << "use_backend b-status if is_options !is_proxy_request is_mgmt_path is_mgmt_id\n";
        txtStream << "use_backend b-status if is_options !is_proxy_request is_stats_path\n";
        txtStream << "use_backend http-proxy if is_proxy_request || is_connect\n";


        //txtStream << "default_backend http-proxy\n";


        txtStream << "backend http-proxy\n";
        txtStream << "mode            http\n";
        txtStream << "timeout connect 5s\n";
        txtStream << "timeout server  30s\n";
        //txtStream << "timeout client  30s\n";
        txtStream << "retries         2\n";
        txtStream << "option          nolinger\n";
        txtStream << "option          httplog\n";
        txtStream << "http-request add-header X-ITNS-PaymentID "+auth+"\n";
        #ifdef Q_OS_WIN
        txtStream << "server hatls " + endpoint + ":" + endpointport + " force-tlsv12 ssl ca-file 'ca.cert.pem'\n";
        #else
        txtStream << "server hatls " + endpoint + ":" + endpointport + " force-tlsv12 ssl ca-file '"+host+"/ca.cert.pem'\n";
        #endif

        #ifdef Q_OS_WIN
        txtStream << "errorfile 503 ha_err_connect.http\n";
        #else
        txtStream << "errorfile 503 "+host+"/ha_err_connect.http\n";
        #endif

        txtStream << "backend b-err\n";
        txtStream << "mode            http\n";
        txtStream << "timeout server  30s\n";
        txtStream << "timeout connect 5s\n";
        //txtStream << "timeout client  30s\n";
        #ifdef Q_OS_WIN
        txtStream << "errorfile 503 ha_err_badid.http\n";
        #else
        txtStream << "errorfile 503 "+host+"/ha_err_badid.http\n";
        #endif

        txtStream << "backend b-status\n";
        txtStream << "mode            http\n";
        txtStream << "timeout server  30s\n";
        txtStream << "timeout connect 5s\n";
        //txtStream << "timeout client  30s\n";
        #ifdef Q_OS_WIN
        txtStream << "errorfile 503 ha_info.http\n";
        #else
        txtStream << "errorfile 503 "+host+"/ha_info.http\n";
        #endif

        txtStream << "backend b-stats\n";
        txtStream << "mode            http\n";
        txtStream << "timeout server  30s\n";
        txtStream << "timeout connect 5s\n";
        txtStream << "server Local 127.0.0.1:8181\n";

        txtStream << "listen  stats\n";
        txtStream << "timeout client  30s\n";
        txtStream << "timeout server  30s\n";
        txtStream << "timeout connect 5s\n";
        txtStream << "bind  127.0.0.1:8181\n";
        txtStream << "mode            http\n";
        txtStream << "stats enable\n";
        txtStream << "stats hide-version\n";
        txtStream << "stats refresh 30s\n";
        txtStream << "stats show-node\n";
        txtStream << "stats uri  /stats\n";

        txtStream << "listen provider\n";
        txtStream << "timeout client  30s\n";
        txtStream << "mode            http\n";
        txtStream << "timeout server  30s\n";
        txtStream << "timeout connect 5s\n";
        txtStream << "errorfile 503 "+host+"/provider.http\n";
        txtStream << "bind 127.0.0.1:8182\n";

        qDebug() << " ----- reading from file ------";

        txtStream.seek(0);
        while(!txtStream.atEnd()){
            qDebug() << txtStream.readLine();
        }
        file.close();

	QString command = "";
        #ifdef Q_OS_WIN
            command="haproxy.exe -f haproxy.cfg";
            WinExec(qPrintable(command),SW_HIDE);
        #else
            //system("trap 'pkill -f haproxy; echo teste haproxy; exit;' INT TERM");
            command=fixedHost+" -f "+host+"/haproxy.cfg";
            qDebug() << command;
            system(qPrintable(command));
        #endif

    }else{
        qDebug() << "could not open the file";
        return;
    }
}

void Haproxy::haproxyCert(const QString &host, const QString &certificate){
    QFile::remove(host+"/ca.cert.pem");
    QFile file (host+"/ca.cert.pem");
    if(!file.exists()){
        qDebug() << file.fileName() << "Certificate does not exists";
    }
    if(file.open(QIODevice::ReadOnly | QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream txtStream(&file);
        qDebug() << "---Writing to file---";
        txtStream << certificate;
        qDebug() << " ----- reading from file ------";
        txtStream.seek(0);
        while(!txtStream.atEnd()){
            qDebug() << txtStream.readLine();
        }
        file.close();
    }else{
        qDebug() << "could not open the file";
        return;
    }
}



void Haproxy::killHAproxy(){
	qDebug() << "kill proxy";
    #ifdef Q_OS_WIN
        WinExec("taskkill /f /im haproxy.exe",SW_HIDE);
    #else
        system("pkill -f haproxy");
    #endif
}

QString Haproxy::verifyHaproxy(const QString &host, const QString &ip, const QString &provider){
    qDebug() << "call verifyHaproxy";

    QtCUrl cUrl;
    cUrl.setTextCodec("Windows-1251");
    //QUrl url("http://remote.lethean/status");

    QtCUrl::Options opt;

    opt[CURLOPT_URL] = "http://remote.lethean/status";
    opt[CURLOPT_PROXY] = "http://"+host+":"+ip+"/";

    //struct curl_slist *list;
    //list = curl_slist_append(NULL, "X-ITNS-MgmtId: 7b08c778af3b28932185d7cc804b0cf399c05c9149613dc149dff5f30c8cd989");
    //curl_easy_setopt(cUrl._curl, CURLOPT_PROXYHEADER, list);
    QStringList list = {"X-ITNS-MgmtId:"+provider};
    opt[CURLOPT_PROXYHEADER] = list;
    opt[CURLOPT_HEADER] = true;

    QString result = cUrl.exec(opt);

    if (cUrl.lastError().isOk()) {
        qDebug() << result;
        return result;
    }
    else {
        qDebug() << QString("Error: %1 Buffer: %2")
            .arg(cUrl.lastError().text())
            .arg(cUrl.errorBuffer());
        return QString("Error: %1 Buffer: %2")
                .arg(cUrl.lastError().text())
                .arg(cUrl.errorBuffer());
    }


}
