#include "SDMBNlocal.h"

int sdmbn_can_serialize(AnalyzerTag::Tag check)
{
    switch(check)
    {
    case AnalyzerTag::PIA_TCP: //1
    case AnalyzerTag::PIA_UDP: //2
    case AnalyzerTag::ICMP: //3
    case AnalyzerTag::TCP: //4
    case AnalyzerTag::UDP: //5
    case AnalyzerTag::DNS: //9
    case AnalyzerTag::FTP: //11
    case AnalyzerTag::HTTP: //13
    case AnalyzerTag::SMTP: //27
    case AnalyzerTag::SSH: //28
    case AnalyzerTag::SSL: //34
    case AnalyzerTag::Teredo: //38
    case AnalyzerTag::File: //39
    case AnalyzerTag::ConnSize: //44
    case AnalyzerTag::ContentLine: //46
    case AnalyzerTag::NVT: //47
    case AnalyzerTag::Zip: //48
        return 1;
        break;
    default:
        fprintf(stderr,"UNHANDLED ANALYZER: %d\n", check);
        return 0;
    }
}
