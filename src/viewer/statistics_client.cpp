// SPDX-License-Identifier: MIT

//! Implements the asynchronous D-Bus client used by the viewer.

#include "statistics_client.h"

#include <utility>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QList>
#include <QMetaType>
#include <QVariant>

namespace inputcounter {

namespace wire {

struct TimeRange final {
  qlonglong start;
  qlonglong end;
};

using TimeRanges = QList<TimeRange>;

QDBusArgument &operator<<(QDBusArgument &argument, const TimeRange &range) {
  argument.beginStructure();
  argument << range.start << range.end;
  argument.endStructure();
  return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                TimeRange &range) {
  argument.beginStructure();
  argument >> range.start >> range.end;
  argument.endStructure();
  return argument;
}

} // namespace wire

} // namespace inputcounter

Q_DECLARE_METATYPE(inputcounter::wire::TimeRange)
Q_DECLARE_METATYPE(inputcounter::wire::TimeRanges)

namespace inputcounter {

namespace {

constexpr char kService[] = "org.fcitx.Fcitx5";
constexpr char kPath[] = "/inputcounter";
constexpr char kInterface[] = "org.fcitx.Fcitx.InputCounter1";
constexpr int kTimeoutMilliseconds = 3000;

QDBusMessage methodCall(const char *method) {
  return QDBusMessage::createMethodCall(
      QString::fromLatin1(kService), QString::fromLatin1(kPath),
      QString::fromLatin1(kInterface), QString::fromLatin1(method));
}

QString errorText(const QDBusMessage &reply) {
  if (!reply.errorMessage().isEmpty()) {
    return reply.errorMessage();
  }
  return QStringLiteral("D-Bus request failed");
}

} // namespace

StatisticsClient::StatisticsClient(QObject *parent) : QObject(parent) {
  qDBusRegisterMetaType<wire::TimeRange>();
  qDBusRegisterMetaType<wire::TimeRanges>();
}

void StatisticsClient::getSummary(const SummaryQuery &summary,
                                  std::function<void(SummaryResult)> callback) {
  auto summaryCall = methodCall("GetSummary");
  summaryCall << static_cast<qlonglong>(summary.todayStart)
              << static_cast<qlonglong>(summary.last24HoursStart)
              << static_cast<qlonglong>(summary.last7DaysStart);

  auto *summaryWatcher =
      new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
                                      summaryCall, kTimeoutMilliseconds),
                                  this);
  connect(summaryWatcher, &QDBusPendingCallWatcher::finished, this,
          [callback =
               std::move(callback)](QDBusPendingCallWatcher *watcher) mutable {
            const auto reply = watcher->reply();
            if (reply.type() == QDBusMessage::ErrorMessage) {
              callback(errorText(reply));
            } else {
              const auto arguments = reply.arguments();
              if (arguments.size() != 6) {
                callback(QStringLiteral("Invalid GetSummary response"));
              } else {
                callback(StatisticsSummary{
                    arguments[0].toULongLong(), arguments[1].toULongLong(),
                    arguments[2].toULongLong(), arguments[3].toULongLong(),
                    arguments[4].toBool(), arguments[5].toLongLong(),
                });
              }
            }
            watcher->deleteLater();
          });
}

void StatisticsClient::getBucketCounts(
    const std::vector<TimeRange> &ranges,
    std::function<void(BucketResult)> callback) {
  auto call = methodCall("GetBucketCounts");
  wire::TimeRanges wireRanges;
  wireRanges.reserve(static_cast<qsizetype>(ranges.size()));
  for (const auto &range : ranges) {
    wireRanges.push_back({range.start, range.end});
  }
  call << QVariant::fromValue(wireRanges);
  auto *watcher = new QDBusPendingCallWatcher(
      QDBusConnection::sessionBus().asyncCall(call, kTimeoutMilliseconds),
      this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [callback =
               std::move(callback)](QDBusPendingCallWatcher *finished) mutable {
            const auto reply = finished->reply();
            if (reply.type() == QDBusMessage::ErrorMessage) {
              callback(errorText(reply));
            } else {
              const auto arguments = reply.arguments();
              if (arguments.size() != 1 ||
                  !arguments[0].canConvert<QDBusArgument>()) {
                callback(
                    QStringLiteral("Invalid GetBucketCounts response"));
              } else {
                auto dbusArgument = arguments[0].value<QDBusArgument>();
                std::vector<std::uint64_t> result;
                dbusArgument.beginArray();
                while (!dbusArgument.atEnd()) {
                  qulonglong value = 0;
                  dbusArgument >> value;
                  result.push_back(value);
                }
                dbusArgument.endArray();
                callback(std::move(result));
              }
            }
            finished->deleteLater();
          });
}

void StatisticsClient::reset(std::function<void(ResetResult)> callback) {
  auto *watcher = new QDBusPendingCallWatcher(
      QDBusConnection::sessionBus().asyncCall(methodCall("Reset"),
                                              kTimeoutMilliseconds),
      this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [callback =
               std::move(callback)](QDBusPendingCallWatcher *finished) mutable {
            const auto reply = finished->reply();
            finished->deleteLater();
            if (reply.type() == QDBusMessage::ErrorMessage) {
              callback(errorText(reply));
              return;
            }
            callback(std::monostate{});
          });
}

} // namespace inputcounter
