add_definitions(-DQT_NO_CAST_FROM_ASCII)
add_definitions(-DQT_NO_CAST_TO_ASCII)
add_definitions(-DTRANSLATION_DOMAIN=\"libkdepim\")

option(KDEPIM_INPROCESS_LDAP "Use in-process rather than KIO slave LDAP" FALSE)
if (KDEPIM_INPROCESS_LDAP)
  add_definitions( -DKDEPIM_INPROCESS_LDAP )
endif ()

set(kdepim_LIB_SRCS
   addressline/addresslineedit/addresseelineedit.cpp
   addressline/addresslineedit/addresseelineedit_p.cpp
   addressline/addresslineedit/addresseelineeditstatic.cpp
   addressline/addresslineedit/baloocompletionemail.cpp
   addressline/addresslineedit/kmailcompletion.cpp
   addressline/blacklistbaloocompletion/blacklistbalooemailcompletiondialog.cpp
   addressline/blacklistbaloocompletion/blacklistbalooemailcompletionwidget.cpp
   addressline/blacklistbaloocompletion/blacklistbalooemaillist.cpp
   addressline/blacklistbaloocompletion/blacklistbalooemailsearchjob.cpp
   addressline/blacklistbaloocompletion/blacklistbalooemailutil.cpp
   addressline/completionorder/completionordereditor.cpp
   addressline/completionorder/completionorderwidget.cpp
   addressline/recentaddress/recentaddresses.cpp
   ldap/ldapclient.cpp
   ldap/ldapclient_debug.cpp
   ldap/ldapclientsearch.cpp
   ldap/ldapclientsearchconfig.cpp
   ldap/ldapsearchdialog.cpp
   libkdepim_debug.cpp
   widgets/progressindicatorlabel.cpp
   widgets/progressindicatorwidget.cpp
)

if (KDEPIM_INPROCESS_LDAP)
  set(kdepim_LIB_SRCS ${kdepim_LIB_SRCS}
    ldap/ldapsession.cpp
    ldap/ldapqueryjob.cpp
  )
endif ()

add_library(zanshinkdepimstatic ${kdepim_LIB_SRCS})

target_link_libraries(zanshinkdepimstatic
  KF5::AkonadiCalendar
  KF5::AkonadiContact
  KF5::AkonadiCore
  KF5::AkonadiSearchPIM
  KF5::AkonadiWidgets
  KF5::Codecs
  KF5::Contacts
  KF5::KCMUtils
  KF5::KCMUtils
  KF5::Ldap
  KF5::Mime
  KF5::Wallet
)

