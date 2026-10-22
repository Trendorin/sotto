#include "core/listener.h"
#include "core/segmenter.h"

Listener::Listener(QObject *parent)
    : QObject(parent)
    , m_worker(new QObject)
{
    m_thread.setObjectName(QStringLiteral("listener"));
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start();
}

Listener::~Listener()
{
    ++m_session;
    m_thread.quit();
    m_thread.wait();
    delete m_segmenter;
}

quint64 Listener::start(int pauseMs)
{
    const quint64 session = ++m_session;
    QMetaObject::invokeMethod(m_worker, [this, session, pauseMs] {
        delete m_segmenter;
        m_segmenter = new Segmenter(pauseMs); // loads the VAD model when it has been downloaded since
        m_segmenterSession = session;
    });
    return session;
}

void Listener::feed(const QList<float> &samples)
{
    const quint64 session = m_session.load();
    QMetaObject::invokeMethod(m_worker, [this, session, samples] {
        if (!current(session))
            return;
        for (const QList<float> &pcm : m_segmenter->feed(samples))
            Q_EMIT phrase(session, pcm);
    });
}

void Listener::finish()
{
    const quint64 session = m_session.load();
    QMetaObject::invokeMethod(m_worker, [this, session] {
        if (!current(session))
            return;
        for (const QList<float> &pcm : m_segmenter->finish())
            Q_EMIT phrase(session, pcm);
        Q_EMIT finished(session, m_segmenter->heardSpeech());
    });
}

void Listener::cancel()
{
    ++m_session;
}

bool Listener::current(quint64 session) const
{
    return m_segmenter && session == m_segmenterSession && session == m_session.load();
}
